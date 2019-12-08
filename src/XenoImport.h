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

#pragma once
#include "resource.h"
#include <matrix3.h>
#include <string>
#include <tchar.h>
#include <vector>
#include <windows.h>

#include "MAXex/win/CFGMacros.h"
#include "../project.h"

static constexpr int XENOMAX_VERSIONINT = XenoMax_VERSION_MAJOR * 100 + XenoMax_VERSION_MINOR;

class XenoImport {
public:
  TSTRING cfgpath;
  const TCHAR *CFGFile;
  HWND hWnd;
  HWND comboHandle;

  NewIDConfigValue(IDC_EDIT_SCALE);
  NewIDConfigIndex(IDC_CB_MOTIONINDEX);

  int windowSize, button1Distance, button2Distance;

  enum ConfigBoolean {
    IDConfigBool(IDC_CH_DEBUGNAME),
    IDConfigBool(IDC_CH_BC5BCHAN),
    IDConfigBool(IDC_CH_TEXTURES),
    IDConfigBool(IDC_CH_TOPNG),
    IDConfigBool(IDC_CH_GLOBAL_FRAMES),
    IDConfigVisible(IDC_CH_BC5BCHAN),
    IDConfigVisible(IDC_CH_TOPNG),
  };

  EnumFlags<uchar, ConfigBoolean> flags;

  struct MotionPair {
    std::string name;
    int ID;
  };

  std::vector<MotionPair> motions;

  XenoImport();
  ~XenoImport() {}

  void BuildCFG();
  void SaveCFG();
  void LoadCFG();

  int SpawnMXMDDialog();
  int SpawnANIDialog();
};

void ShowAboutDLG(HWND hWnd);

static const Matrix3 corMat = {
    {1.f, 0.f, 0.f}, {0.f, 0.f, 1.f}, {0.f, -1.f, 0.f}, {0.f, 0.f, 0.f}};