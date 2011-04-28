// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/web_resource/gpu_blacklist_updater.h"

#include "base/logging.h"
#include "base/values.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/prefs/pref_service.h"
#include "chrome/browser/prefs/scoped_user_pref_update.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/common/chrome_version_info.h"
#include "chrome/common/pref_names.h"
#include "content/browser/browser_thread.h"
#include "content/browser/gpu_blacklist.h"
#include "content/browser/gpu_data_manager.h"
#include "content/common/notification_type.h"
#include "grit/browser_resources.h"
#include "ui/base/resource/resource_bundle.h"

namespace {

// Delay on first fetch so we don't interfere with startup.
static const int kStartGpuBlacklistFetchDelay = 6000;

// Delay between calls to update the gpu blacklist (48 hours).
static const int kCacheUpdateDelay = 48 * 60 * 60 * 1000;

}  // namespace

const char* GpuBlacklistUpdater::kDefaultGpuBlacklistURL =
    "https://dl.google.com/dl/edgedl/chrome/gpu/software_rendering_list.json";

GpuBlacklistUpdater::GpuBlacklistUpdater()
    : WebResourceService(ProfileManager::GetDefaultProfile(),
                         g_browser_process->local_state(),
                         GpuBlacklistUpdater::kDefaultGpuBlacklistURL,
                         false,  // don't append locale to URL
                         NotificationType::NOTIFICATION_TYPE_COUNT,
                         prefs::kGpuBlacklistUpdate,
                         kStartGpuBlacklistFetchDelay,
                         kCacheUpdateDelay) {
  prefs_->RegisterDictionaryPref(prefs::kGpuBlacklist);
  const DictionaryValue* gpu_blacklist_cache =
      prefs_->GetDictionary(prefs::kGpuBlacklist);
  LoadGpuBlacklist(*gpu_blacklist_cache);
}

GpuBlacklistUpdater::~GpuBlacklistUpdater() { }

void GpuBlacklistUpdater::Unpack(const DictionaryValue& parsed_json) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  prefs_->Set(prefs::kGpuBlacklist, parsed_json);
  LoadGpuBlacklist(parsed_json);
}

void GpuBlacklistUpdater::LoadGpuBlacklist(
    const DictionaryValue& gpu_blacklist_cache) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  scoped_ptr<GpuBlacklist> gpu_blacklist;
  // We first load it from the browser resources, and then check if the cached
  // version is more up-to-date.
  static const base::StringPiece gpu_blacklist_json(
      ResourceBundle::GetSharedInstance().GetRawDataResource(
          IDR_GPU_BLACKLIST));
  chrome::VersionInfo version_info;
  std::string chrome_version_string =
      version_info.is_valid() ? version_info.Version() : "0";
  gpu_blacklist.reset(new GpuBlacklist(chrome_version_string));
  if (!gpu_blacklist->LoadGpuBlacklist(gpu_blacklist_json.as_string(), true))
    return;

  uint16 version_major, version_minor;
  bool succeed = gpu_blacklist->GetVersion(&version_major, &version_minor);
  DCHECK(succeed);
  VLOG(1) << "Using software rendering list version "
          << version_major << "." << version_minor;

  uint16 cached_version_major, cached_version_minor;
  if (GpuBlacklist::GetVersion(gpu_blacklist_cache,
                               &cached_version_major,
                               &cached_version_minor)) {
    if (gpu_blacklist.get() != NULL) {
      uint16 current_version_major, current_version_minor;
      if (gpu_blacklist->GetVersion(&current_version_major,
                                    &current_version_minor) &&
          (cached_version_major > current_version_major ||
           (cached_version_major == current_version_major &&
            cached_version_minor > current_version_minor))) {
        chrome::VersionInfo version_info;
        std::string chrome_version_string =
            version_info.is_valid() ? version_info.Version() : "0";
        scoped_ptr<GpuBlacklist> updated_list(
            new GpuBlacklist(chrome_version_string));
        if (updated_list->LoadGpuBlacklist(gpu_blacklist_cache, true)) {
          gpu_blacklist.reset(updated_list.release());
          VLOG(1) << "Using software rendering list version "
              << cached_version_major << "." << cached_version_minor;
        }
      }
    }
  }

  // Need to initialize GpuDataManager to load the current GPU blacklist,
  // collect preliminary GPU info, and run through GPU blacklist.
  GpuDataManager* gpu_data_manager = GpuDataManager::GetInstance();
  gpu_data_manager->UpdateGpuBlacklist(gpu_blacklist.release());
}
