// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/bookmarks/bookmark_context_menu_controller.h"

#include "app/l10n_util.h"
#include "base/compiler_specific.h"
#include "chrome/browser/bookmarks/bookmark_editor.h"
#include "chrome/browser/bookmarks/bookmark_folder_editor_controller.h"
#include "chrome/browser/bookmarks/bookmark_manager.h"
#include "chrome/browser/bookmarks/bookmark_model.h"
#include "chrome/browser/bookmarks/bookmark_utils.h"
#include "chrome/browser/browser.h"
#include "chrome/browser/browser_list.h"
#include "chrome/browser/metrics/user_metrics.h"
#include "chrome/browser/pref_service.h"
#include "chrome/browser/profile.h"
#include "chrome/browser/tab_contents/page_navigator.h"
#include "chrome/common/pref_names.h"
#include "grit/generated_resources.h"

namespace {

// Returns true if the specified node is of type URL, or has a descendant
// of type URL.
bool NodeHasURLs(const BookmarkNode* node) {
  if (node->is_url())
    return true;

  for (int i = 0; i < node->GetChildCount(); ++i) {
    if (NodeHasURLs(node->GetChild(i)))
      return true;
  }
  return false;
}

// SelectOnCreationHandler ----------------------------------------------------

// Used when adding a new bookmark. If a new bookmark is created it is selected
// in the bookmark manager.
class SelectOnCreationHandler : public BookmarkEditor::Handler {
 public:
  explicit SelectOnCreationHandler(Profile* profile) : profile_(profile) {
  }

  virtual void NodeCreated(const BookmarkNode* new_node) {
// TODO(viettrungluu): I don't know if this is really needed, but it'll be
// deleted soon.
#if defined(OS_WIN)
    BookmarkManager::SelectInTree(profile_, new_node);
#endif
  }

 private:
  Profile* profile_;

  DISALLOW_COPY_AND_ASSIGN(SelectOnCreationHandler);
};

}  // namespace

BookmarkContextMenuController::BookmarkContextMenuController(
    gfx::NativeWindow parent_window,
    BookmarkContextMenuControllerDelegate* delegate,
    Profile* profile,
    PageNavigator* navigator,
    const BookmarkNode* parent,
    const std::vector<const BookmarkNode*>& selection,
    ConfigurationType configuration)
    : parent_window_(parent_window),
      delegate_(delegate),
      profile_(profile),
      navigator_(navigator),
      parent_(parent),
      selection_(selection),
      configuration_(configuration),
      model_(profile->GetBookmarkModel()) {
  DCHECK(profile_);
  DCHECK(model_->IsLoaded());
  menu_model_.reset(new menus::SimpleMenuModel(this));
  model_->AddObserver(this);

  BuildMenu();
}

BookmarkContextMenuController::~BookmarkContextMenuController() {
  if (model_)
    model_->RemoveObserver(this);
}

void BookmarkContextMenuController::BuildMenu() {
  if (configuration_ != BOOKMARK_MANAGER_ORGANIZE_MENU) {
    if (selection_.size() == 1 && selection_[0]->is_url()) {
      AddItem(IDS_BOOMARK_BAR_OPEN_ALL,
              IDS_BOOMARK_BAR_OPEN_IN_NEW_TAB);
      AddItem(IDS_BOOMARK_BAR_OPEN_ALL_NEW_WINDOW,
              IDS_BOOMARK_BAR_OPEN_IN_NEW_WINDOW);
      AddItem(IDS_BOOMARK_BAR_OPEN_ALL_INCOGNITO,
              IDS_BOOMARK_BAR_OPEN_INCOGNITO);
    } else {
      AddItem(IDS_BOOMARK_BAR_OPEN_ALL);
      AddItem(IDS_BOOMARK_BAR_OPEN_ALL_NEW_WINDOW);
      AddItem(IDS_BOOMARK_BAR_OPEN_ALL_INCOGNITO);
    }
    AddSeparator();
  }

  if (selection_.size() == 1 && selection_[0]->is_folder()) {
    AddItem(IDS_BOOKMARK_BAR_RENAME_FOLDER);
  } else {
    AddItem(IDS_BOOKMARK_BAR_EDIT);
  }

// TODO(viettrungluu): I don't know if this is really needed, but it'll be
// deleted soon.
#if defined(OS_WIN)
  if (configuration_ == BOOKMARK_MANAGER_TABLE_OTHER ||
      configuration_ == BOOKMARK_MANAGER_ORGANIZE_MENU_OTHER) {
    AddItem(IDS_BOOKMARK_MANAGER_SHOW_IN_FOLDER);
  }
#endif

  AddSeparator();
  AddItem(IDS_CUT);
  AddItem(IDS_COPY);
  AddItem(IDS_PASTE);

  AddSeparator();
  AddItem(IDS_BOOKMARK_BAR_REMOVE);

  if (configuration_ == BOOKMARK_MANAGER_ORGANIZE_MENU) {
    AddSeparator();
    AddItem(IDS_BOOKMARK_MANAGER_SORT);
  }

  AddSeparator();

  AddItem(IDS_BOOMARK_BAR_ADD_NEW_BOOKMARK);
  AddItem(IDS_BOOMARK_BAR_NEW_FOLDER);

  if (configuration_ == BOOKMARK_BAR) {
    AddSeparator();
    AddItem(IDS_BOOKMARK_MANAGER);
    AddCheckboxItem(IDS_BOOMARK_BAR_ALWAYS_SHOW);
  }
}

void BookmarkContextMenuController::AddItem(int id) {
  menu_model_->AddItem(id, l10n_util::GetStringUTF16(id));
}

void BookmarkContextMenuController::AddItem(int id, int localization_id) {
  menu_model_->AddItemWithStringId(id, localization_id);
}

void BookmarkContextMenuController::AddSeparator() {
  menu_model_->AddSeparator();
}

void BookmarkContextMenuController::AddCheckboxItem(int id) {
  menu_model_->AddCheckItemWithStringId(id, id);
}

void BookmarkContextMenuController::ExecuteCommand(int id) {
  if (delegate_)
    delegate_->WillExecuteCommand();

  switch (id) {
    case IDS_BOOMARK_BAR_OPEN_ALL:
    case IDS_BOOMARK_BAR_OPEN_ALL_INCOGNITO:
    case IDS_BOOMARK_BAR_OPEN_ALL_NEW_WINDOW: {
      WindowOpenDisposition initial_disposition;
      if (id == IDS_BOOMARK_BAR_OPEN_ALL) {
        initial_disposition = NEW_FOREGROUND_TAB;
        UserMetrics::RecordAction(
            UserMetricsAction("BookmarkBar_ContextMenu_OpenAll"),
            profile_);
      } else if (id == IDS_BOOMARK_BAR_OPEN_ALL_NEW_WINDOW) {
        initial_disposition = NEW_WINDOW;
        UserMetrics::RecordAction(
                UserMetricsAction("BookmarkBar_ContextMenu_OpenAllInNewWindow"),
                profile_);
      } else {
        initial_disposition = OFF_THE_RECORD;
        UserMetrics::RecordAction(
            UserMetricsAction("BookmarkBar_ContextMenu_OpenAllIncognito"),
            profile_);
      }
      bookmark_utils::OpenAll(parent_window_, profile_, navigator_, selection_,
                              initial_disposition);
      break;
    }

    case IDS_BOOKMARK_BAR_RENAME_FOLDER:
    case IDS_BOOKMARK_BAR_EDIT:
      UserMetrics::RecordAction(
          UserMetricsAction("BookmarkBar_ContextMenu_Edit"),
          profile_);

      if (selection_.size() != 1) {
        NOTREACHED();
        break;
      }

      if (selection_[0]->is_url()) {
        BookmarkEditor::Configuration editor_config;
        if (configuration_ == BOOKMARK_BAR)
          editor_config = BookmarkEditor::SHOW_TREE;
        else
          editor_config = BookmarkEditor::NO_TREE;
        BookmarkEditor::Show(parent_window_, profile_, parent_,
                             BookmarkEditor::EditDetails(selection_[0]),
                             editor_config, NULL);
      } else {
        BookmarkFolderEditorController::Show(profile_, parent_window_,
            selection_[0], -1, BookmarkFolderEditorController::NONE);
      }
      break;

    case IDS_BOOKMARK_BAR_REMOVE: {
      UserMetrics::RecordAction(
          UserMetricsAction("BookmarkBar_ContextMenu_Remove"),
          profile_);

      for (size_t i = 0; i < selection_.size(); ++i) {
        model_->Remove(selection_[i]->GetParent(),
                      selection_[i]->GetParent()->IndexOfChild(selection_[i]));
      }
      selection_.clear();
      break;
    }

    case IDS_BOOMARK_BAR_ADD_NEW_BOOKMARK: {
      UserMetrics::RecordAction(
          UserMetricsAction("BookmarkBar_ContextMenu_Add"),
          profile_);

      BookmarkEditor::Configuration editor_config;
      BookmarkEditor::Handler* handler = NULL;
      if (configuration_ == BOOKMARK_BAR) {
        editor_config = BookmarkEditor::SHOW_TREE;
      } else {
        editor_config = BookmarkEditor::NO_TREE;
        // This is owned by the BookmarkEditorView.
        handler = new SelectOnCreationHandler(profile_);
      }
      // TODO: this should honor the index from GetParentForNewNodes.
      BookmarkEditor::Show(
          parent_window_, profile_,
          bookmark_utils::GetParentForNewNodes(parent_, selection_, NULL),
          BookmarkEditor::EditDetails(), editor_config,
          handler);
      break;
    }

    case IDS_BOOMARK_BAR_NEW_FOLDER: {
      UserMetrics::RecordAction(
          UserMetricsAction("BookmarkBar_ContextMenu_NewFolder"),
          profile_);
      int index;
      const BookmarkNode* parent =
          bookmark_utils::GetParentForNewNodes(parent_, selection_, &index);
      uint32 details = BookmarkFolderEditorController::IS_NEW;
      if (configuration_ != BOOKMARK_BAR)
        details |= BookmarkFolderEditorController::SHOW_IN_MANAGER;
      BookmarkFolderEditorController::Show(profile_, parent_window_, parent,
                                           index, details);
      break;
    }

    case IDS_BOOMARK_BAR_ALWAYS_SHOW:
      bookmark_utils::ToggleWhenVisible(profile_);
      break;

// TODO(viettrungluu): I don't know if this is really needed, but it'll be
// deleted soon.
#if defined(OS_WIN)
    case IDS_BOOKMARK_MANAGER_SHOW_IN_FOLDER:
      UserMetrics::RecordAction(
          UserMetricsAction("BookmarkBar_ContextMenu_ShowInFolder"),
          profile_);

      if (selection_.size() != 1) {
        NOTREACHED();
        break;
      }

      BookmarkManager::SelectInTree(profile_, selection_[0]);
      break;
#endif

    case IDS_BOOKMARK_MANAGER:
      UserMetrics::RecordAction(UserMetricsAction("ShowBookmarkManager"),
                                profile_);
      {
        Browser* browser = BrowserList::GetLastActiveWithProfile(profile_);
        if (browser)
          browser->OpenBookmarkManager();
        else
          NOTREACHED();
      }
      break;

    case IDS_BOOKMARK_MANAGER_SORT:
      UserMetrics::RecordAction(UserMetricsAction("BookmarkManager_Sort"),
                                profile_);
      model_->SortChildren(parent_);
      break;

    case IDS_CUT:
      bookmark_utils::CopyToClipboard(model_, selection_, true);
      break;

    case IDS_COPY:
      bookmark_utils::CopyToClipboard(model_, selection_, false);
      break;

    case IDS_PASTE: {
      int index;
      const BookmarkNode* paste_target =
          bookmark_utils::GetParentForNewNodes(parent_, selection_, &index);
      if (!paste_target)
        return;

      bookmark_utils::PasteFromClipboard(model_, parent_, index);
      break;
    }

    default:
      NOTREACHED();
  }

  if (delegate_)
    delegate_->DidExecuteCommand();
}

bool BookmarkContextMenuController::IsCommandIdChecked(int command_id) const {
  DCHECK(command_id == IDS_BOOMARK_BAR_ALWAYS_SHOW);
  return profile_->GetPrefs()->GetBoolean(prefs::kShowBookmarkBar);
}

bool BookmarkContextMenuController::IsCommandIdEnabled(int command_id) const {
  bool is_root_node =
      (selection_.size() == 1 &&
       selection_[0]->GetParent() == model_->root_node());
  switch (command_id) {
    case IDS_BOOMARK_BAR_OPEN_INCOGNITO:
      return !profile_->IsOffTheRecord();

    case IDS_BOOMARK_BAR_OPEN_ALL_INCOGNITO:
      return HasURLs() && !profile_->IsOffTheRecord();

    case IDS_BOOMARK_BAR_OPEN_ALL:
    case IDS_BOOMARK_BAR_OPEN_ALL_NEW_WINDOW:
      return HasURLs();

    case IDS_BOOKMARK_BAR_RENAME_FOLDER:
    case IDS_BOOKMARK_BAR_EDIT:
      return selection_.size() == 1 && !is_root_node;

    case IDS_BOOKMARK_BAR_REMOVE:
      return !selection_.empty() && !is_root_node;

// TODO(viettrungluu): I don't know if this is really needed, but it'll be
// deleted soon.
#if defined(OS_WIN)
    case IDS_BOOKMARK_MANAGER_SHOW_IN_FOLDER:
      return (configuration_ == BOOKMARK_MANAGER_TABLE_OTHER ||
              configuration_ == BOOKMARK_MANAGER_ORGANIZE_MENU_OTHER) &&
             selection_.size() == 1;
#endif

    case IDS_BOOKMARK_MANAGER_SORT:
      return parent_ && parent_ != model_->root_node();

    case IDS_BOOMARK_BAR_NEW_FOLDER:
    case IDS_BOOMARK_BAR_ADD_NEW_BOOKMARK:
      return bookmark_utils::GetParentForNewNodes(
          parent_, selection_, NULL) != NULL;

    case IDS_COPY:
    case IDS_CUT:
      return selection_.size() > 0 && !is_root_node;

    case IDS_PASTE:
      // Paste to selection from the Bookmark Bar, to parent_ everywhere else
      return (configuration_ == BOOKMARK_BAR && !selection_.empty() &&
             bookmark_utils::CanPasteFromClipboard(selection_[0])) ||
             bookmark_utils::CanPasteFromClipboard(parent_);
  }
  return true;
}

void BookmarkContextMenuController::BookmarkModelChanged() {
  if (delegate_)
    delegate_->CloseMenu();
}

bool BookmarkContextMenuController::HasURLs() const {
  for (size_t i = 0; i < selection_.size(); ++i) {
    if (NodeHasURLs(selection_[i]))
      return true;
  }
  return false;
}
