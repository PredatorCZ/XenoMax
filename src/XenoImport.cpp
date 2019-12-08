/*      Xenoblade Tool for 3ds Max
        Copyright(C) 2017-2019 Lukas Cone

        This program is free software : you can redistribute it and / or modify
        it under the terms of the GNU General Public License as published by
        the Free Software Foundation, either version 3 of the License, or
        (at your option) any later version.

        This program is distributed in the hope that it will be useful,
        but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
        GNU General Public License for more details.

        You should have received a copy of the GNU General Public License
        along with this program.If not, see <https://www.gnu.org/licenses/>.
*/

#include "XenoImport.h"
#include "datas/esstring.h"
#include <3dsmaxport.h>
#include <IPathConfigMgr.h>
#include <iparamm2.h>

extern HINSTANCE hInstance;
const TCHAR _name[] = _T("Xenoblade Tool");
const TCHAR _info[] =
    _T("\n" XenoMax_COPYRIGHT "\nVersion " XenoMax_VERSION);
const TCHAR _license[] =
    _T("Xenoblade Tool uses XenoLib, Copyright(C) 2017-2019 Lukas Cone.");
const TCHAR _homePage[] = _T("https://lukascone.wordpress.com/2019/06/04/")
                          _T("xenoblade-chronicles-3ds-max-plugin/");

#include "MAXex/win/AboutDlg.h"

XenoImport::XenoImport()
    : CFGFile(nullptr), hWnd(nullptr), IDConfigValue(IDC_EDIT_SCALE)(145.f),
      flags(IDC_CH_DEBUGNAME_checked) {
  LoadCFG();
}

void XenoImport::LoadCFG() {
  BuildCFG();
  TCHAR buffer[CFGBufferSize];

  GetCFGValue(IDC_EDIT_SCALE);
  GetCFGIndex(IDC_CB_MOTIONINDEX);
  GetCFGChecked(IDC_CH_DEBUGNAME);
  GetCFGChecked(IDC_CH_BC5BCHAN);
  GetCFGChecked(IDC_CH_TEXTURES);
  GetCFGChecked(IDC_CH_TOPNG);
  GetCFGChecked(IDC_CH_GLOBAL_FRAMES);
  GetCFGEnabled(IDC_CH_BC5BCHAN);
  GetCFGEnabled(IDC_CH_TOPNG);
}

void XenoImport::BuildCFG() {
  cfgpath = IPathConfigMgr::GetPathConfigMgr()->GetDir(APP_PLUGCFG_DIR);
  cfgpath.append(_T("\\XenoImpSettings.ini"));
  CFGFile = cfgpath.c_str();
}

void XenoImport::SaveCFG() {
  BuildCFG();

  TCHAR buffer[CFGBufferSize];
  SetCFGValue(IDC_EDIT_SCALE);
  SetCFGIndex(IDC_CB_MOTIONINDEX);
  SetCFGChecked(IDC_CH_DEBUGNAME);
  SetCFGChecked(IDC_CH_BC5BCHAN);
  SetCFGChecked(IDC_CH_TEXTURES);
  SetCFGChecked(IDC_CH_GLOBAL_FRAMES);
  SetCFGChecked(IDC_CH_TOPNG);
  SetCFGEnabled(IDC_CH_BC5BCHAN);
  SetCFGEnabled(IDC_CH_TOPNG);

  WriteText(hkpresetgroup, _T("Xenoblade"), CFGFile, _T("Name"));
  WriteValue(hkpresetgroup, IDConfigValue(IDC_EDIT_SCALE), CFGFile, buffer,
             _T("Scale"));
  WriteCorrectionMatrix(corMat, CFGFile, buffer);
}

static INT_PTR CALLBACK DialogCallbacks(HWND hWnd, UINT message, WPARAM wParam,
                                        LPARAM lParam) {
  XenoImport *imp = DLGetWindowLongPtr<XenoImport *>(hWnd);

  switch (message) {
  case WM_INITDIALOG: {
    CenterWindow(hWnd, GetParent(hWnd));
    imp = reinterpret_cast<XenoImport *>(lParam);
    DLSetWindowLongPtr(hWnd, lParam);
    imp->hWnd = hWnd;
    imp->LoadCFG();
    SetupIntSpinner(hWnd, IDC_SPIN_SCALE, IDC_EDIT_SCALE, 0, 5000,
                    imp->IDC_EDIT_SCALE_value);
    SetWindowText(hWnd, _T("Xenoblade Import v" XenoMax_VERSION));

    HWND butt = GetDlgItem(hWnd, IDC_BT_DONE);
    RECT buttRect;
    RECT rekt;

    GetWindowRect(hWnd, &rekt);
    imp->windowSize = rekt.right - rekt.left;
    GetClientRect(hWnd, &rekt);

    GetWindowRect(butt, &buttRect);
    MapWindowPoints(HWND_DESKTOP, hWnd, (LPPOINT)&buttRect, 2);

    imp->button1Distance = rekt.right - buttRect.left;

    butt = GetDlgItem(hWnd, IDC_BT_CANCEL);
    GetWindowRect(butt, &buttRect);
    MapWindowPoints(HWND_DESKTOP, hWnd, (LPPOINT)&buttRect, 2);
    imp->button2Distance = rekt.right - buttRect.left;

    imp->comboHandle = GetDlgItem(hWnd, IDC_CB_MOTIONINDEX);

    for (auto &p : imp->motions)
      SendMessage(imp->comboHandle, CB_ADDSTRING, 0,
                  (LPARAM)esString(p.name).ToTString<TCHAR>().c_str());

    SendMessage(imp->comboHandle, CB_SETCURSEL,
                imp->IDC_CB_MOTIONINDEX_index >= imp->motions.size()
                    ? 0
                    : imp->IDC_CB_MOTIONINDEX_index,
                0);

    return TRUE;
  }

  case WM_SIZE: {
    HWND butt = GetDlgItem(hWnd, IDC_BT_DONE);
    RECT buttRect;
    RECT rekt;

    GetClientRect(hWnd, &rekt);
    GetWindowRect(butt, &buttRect);
    MapWindowPoints(HWND_DESKTOP, hWnd, (LPPOINT)&buttRect, 2);

    SetWindowPos(butt, 0, rekt.right - imp->button1Distance, buttRect.top, 0, 0,
                 SWP_NOZORDER | SWP_NOSIZE | SWP_NOOWNERZORDER);

    butt = GetDlgItem(hWnd, IDC_BT_CANCEL);
    SetWindowPos(butt, 0, rekt.right - imp->button2Distance, buttRect.top, 0, 0,
                 SWP_NOZORDER | SWP_NOSIZE | SWP_NOOWNERZORDER);

    butt = GetDlgItem(hWnd, IDC_CB_MOTIONINDEX);
    GetClientRect(butt, &buttRect);

    SetWindowPos(butt, 0, 0, 0, rekt.right - rekt.left - 15, buttRect.bottom,
                 SWP_NOZORDER | SWP_NOMOVE | SWP_NOOWNERZORDER);

    break;
  }

  case WM_GETMINMAXINFO: {
    LPMINMAXINFO lpMMI = (LPMINMAXINFO)lParam;
    lpMMI->ptMinTrackSize.x = imp->windowSize;
    break;
  }

  case WM_NCHITTEST: {
    LRESULT lRes = DefWindowProc(hWnd, message, wParam, lParam);

    if (lRes == HTBOTTOMLEFT || lRes == HTBOTTOMRIGHT || lRes == HTTOPLEFT ||
        lRes == HTTOPRIGHT || lRes == HTTOP || lRes == HTBOTTOM ||
        lRes == HTSIZE)
      return TRUE;

    return FALSE;
  }

  case WM_CLOSE:
    EndDialog(hWnd, 0);
    imp->SaveCFG();
    return 1;

  case WM_COMMAND:
    switch (LOWORD(wParam)) {
    case IDOK:
      EndDialog(hWnd, 1);
      imp->SaveCFG();
      return 1;
    case IDC_BT_ABOUT:
      ShowAboutDLG(hWnd);
      return 1;
    case IDCANCEL:
      EndDialog(hWnd, 0);
      imp->SaveCFG();
      return 1;

      MSGCheckbox(IDC_CH_DEBUGNAME);
      break;
      MSGCheckbox(IDC_CH_TEXTURES);
      MSGEnable(IDC_CH_TEXTURES, IDC_CH_TOPNG);
      MSGEnableEnabled(IDC_CH_TOPNG, IDC_CH_BC5BCHAN);
      break;

      MSGCheckbox(IDC_CH_BC5BCHAN);
      break;

      MSGCheckbox(IDC_CH_GLOBAL_FRAMES);
      break;

      MSGCheckbox(IDC_CH_TOPNG);
      MSGEnable(IDC_CH_TOPNG, IDC_CH_BC5BCHAN);
      break;
    }

  case CC_SPINNER_CHANGE:
    switch (LOWORD(wParam)) {
    case IDC_SPIN_SCALE:
      imp->IDC_EDIT_SCALE_value =
          reinterpret_cast<ISpinnerControl *>(lParam)->GetFVal();
      break;
    }
  case IDC_CB_MOTIONINDEX: {
    switch (HIWORD(wParam)) {
    case CBN_SELCHANGE: {
      const LRESULT curSel = SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0);
      imp->IDC_CB_MOTIONINDEX_index = curSel;
      return TRUE;
    } break;
    }
    break;
  }
  }
  return 0;
}

int XenoImport::SpawnMXMDDialog() {
  return DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_MXMD), GetActiveWindow(),
                        DialogCallbacks, (LPARAM)this);
}

int XenoImport::SpawnANIDialog() {
  return DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_ANIMSKEL),
                        GetActiveWindow(), DialogCallbacks, (LPARAM)this);
}