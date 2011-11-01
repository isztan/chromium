// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef VIEWS_CONTROLS_PROGRESS_BAR_H_
#define VIEWS_CONTROLS_PROGRESS_BAR_H_
#pragma once

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "views/view.h"

namespace views {

// Progress bar is a control that indicates progress visually.
class VIEWS_EXPORT ProgressBar : public View {
 public:
  // The value range defaults to [0.0, 1.0].
  ProgressBar();
  virtual ~ProgressBar();

  double current_value() const { return current_value_; }

  // Sets the inclusive range of values to be displayed.  Values outside of the
  // range will be capped when displayed.
  void SetDisplayRange(double min_display_value, double max_display_value);

  // Sets the current value.  Values outside of the range [min_display_value_,
  // max_display_value_] will be stored unmodified and capped for display.
  void SetValue(double value);

  // Sets the tooltip text.  Default behavior for a progress bar is to show no
  // tooltip on mouse hover. Calling this lets you set a custom tooltip.  To
  // revert to default behavior, call this with an empty string.
  void SetTooltipText(const string16& tooltip_text);

  // Overridden from View:
  virtual bool GetTooltipText(const gfx::Point& p,
                              string16* tooltip) const OVERRIDE;
  virtual void GetAccessibleState(ui::AccessibleViewState* state) OVERRIDE;

 private:
  static const char kViewClassName[];

  // Overridden from View:
  virtual gfx::Size GetPreferredSize() OVERRIDE;
  virtual std::string GetClassName() const OVERRIDE;
  virtual void OnPaint(gfx::Canvas* canvas) OVERRIDE;

  // Inclusive range used when displaying values.
  double min_display_value_;
  double max_display_value_;

  // Current value.  May be outside of [min_display_value_, max_display_value_].
  double current_value_;

  // Tooltip text.
  string16 tooltip_text_;

  DISALLOW_COPY_AND_ASSIGN(ProgressBar);
};

}  // namespace views

#endif  // VIEWS_CONTROLS_PROGRESS_BAR_H_
