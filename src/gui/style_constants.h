#pragma once

#include <QString>

namespace gis_ai::gui {

namespace Color {

constexpr const char* kWindowBg          = "#E8ECF1";
constexpr const char* kPagePanelBg       = "#F5F6F8";
constexpr const char* kCardBg            = "#FFFFFF";
constexpr const char* kCardBorder        = "#E2E5EA";
constexpr const char* kCardShadow        = "rgba(15, 23, 42, 0.08)";
constexpr const char* kSidebarBg         = "#1E2A36";
constexpr const char* kSidebarPanel      = "#1E2A36";
constexpr const char* kSidebarSelected   = "#1E3A5F";
constexpr const char* kSidebarHover      = "#263544";
constexpr const char* kSidebarIndicator  = "#2E7EC9";
constexpr const char* kSidebarText       = "#BCC4CF";
constexpr const char* kSidebarMuted      = "#8B9099";
constexpr const char* kPrimary           = "#2E7EC9";
constexpr const char* kPrimaryHover      = "#3590DA";
constexpr const char* kPrimaryPressed    = "#2575C0";
constexpr const char* kPrimaryLight      = "#E8F2FB";
constexpr const char* kPrimaryDeep       = "#1F6AB5";
constexpr const char* kSuccess           = "#1F9D68";
constexpr const char* kSuccessBg         = "#E8F7F0";
constexpr const char* kWarning           = "#D97706";
constexpr const char* kWarningBg         = "#FFF4DF";
constexpr const char* kError             = "#DC4C3E";
constexpr const char* kErrorBg           = "#FFF0EE";
constexpr const char* kTextPrimary       = "#15253A";
constexpr const char* kTextSecondary     = "#4F637A";
constexpr const char* kTextMuted         = "#7E92A8";
constexpr const char* kDivider           = "#E3EAF2";
constexpr const char* kInputBorder       = "#D3DDE8";
constexpr const char* kInputBg           = "#FDFEFF";
constexpr const char* kInputFocusBorder  = "#2F7CF6";
constexpr const char* kBrowseBtnBg       = "#F4F7FB";
constexpr const char* kBrowseBtnHover    = "#E8EEF6";
constexpr const char* kProgressTrack     = "#E7EEF7";
constexpr const char* kProgressFill      = "#2F7CF6";
constexpr const char* kDisabledBg        = "#E9EEF5";
constexpr const char* kDisabledText      = "#90A0B2";

}

namespace Size {

constexpr int kSidebarWidth         = 250;
constexpr int kSidebarMinWidth      = 240;
constexpr int kWindowMinWidth       = 1080;
constexpr int kWindowMinHeight      = 720;
constexpr int kWindowDefaultWidth   = 1380;
constexpr int kWindowDefaultHeight  = 880;
constexpr int kCardRadius           = 10;
constexpr int kCardPadding          = 18;
constexpr int kCardSpacing          = 16;
constexpr int kInputRadius          = 6;
constexpr int kInputMinHeight       = 36;
constexpr int kInputPaddingH        = 14;
constexpr int kButtonRadius         = 6;
constexpr int kButtonMinHeight      = 40;
constexpr int kButtonMinWidth       = 118;
constexpr int kProgressHeight       = 8;
constexpr int kProgressRadius       = 4;
constexpr int kLabelInputRatio      = 2;
constexpr double kBadgeIconRatio     = 0.55;

}

QString globalStyleSheet();

QString sidebarStyleSheet();

}
