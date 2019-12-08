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

#include <thread>

#include <IPathConfigMgr.h>
#include <MeshNormalSpec.h>
#include <ilayer.h>
#include <ilayermanager.h>
#include <iskin.h>
#include <stdmat.h>
#include <triobj.h>

#include "MAXex/Maps.h"
#include <../samples/modifiers/morpher/include/MorpherApi.h>

#include "BC.h"
#include "MXMD.h"
#include "SAR.h"
#include "XenoImport.h"
#include "XenoMax.h"

#include "MAXex/NodeSuffix.h"
#include "datas/esstring.h"
#include "datas/fileinfo.hpp"
#include "datas/masterprinter.hpp"

#define XenoImp_CLASS_ID Class_ID(0xabe2469a, 0xa9eaf87a)
#define HavokImport_CLASS_ID Class_ID(0xad115395, 0x924c02c0)
static const TCHAR _className[] = _T("XenoImp");

class XenoImp : public SceneImport, XenoImport {
public:
  // Constructor/Destructor
  XenoImp();
  virtual ~XenoImp();

  virtual int ExtCount();          // Number of extensions supported
  virtual const TCHAR *Ext(int n); // Extension #n (i.e. "3DS")
  virtual const TCHAR *
  LongDesc(); // Long ASCII description (i.e. "Autodesk 3D Studio File")
  virtual const TCHAR *
  ShortDesc(); // Short ASCII description (i.e. "3D Studio")
  virtual const TCHAR *AuthorName();       // ASCII Author name
  virtual const TCHAR *CopyrightMessage(); // ASCII Copyright message
  virtual const TCHAR *OtherMessage1();    // Other message #1
  virtual const TCHAR *OtherMessage2();    // Other message #2
  virtual unsigned int Version();    // Version number * 100 (i.e. v3.01 = 301)
  virtual void ShowAbout(HWND hWnd); // Show DLL's "About..." box
  virtual int DoImport(const TCHAR *name, ImpInterface *i, Interface *gi,
                       BOOL suppressPrompts = FALSE); // Import file

  std::vector<INode *> remapNodes;
  std::vector<StdMat *> outMats;
  std::vector<BitmapTex *> texmaps;

  void LoadSkeleton(BCSKEL *skel);
  void LoadAnimation(BCANIM *anim);
  void LoadModels(MXMD *model);
  INodeTab LoadMeshes(MXMD *model, MXMDModel::Ptr &mdl, int curGroup);
  int LoadTextures(MXMD *model);
  void LoadMaterials(MXMD *model);
  int LoadInstances(MXMD *model);
  void LoadModelPose(MXMDModel::Ptr &model);
  void ApplySkin(MXMDGeomBuffers::Ptr &buff, MXMDMeshObject::Ptr &mesh,
                 INodeSuffixer &nde, Face *mfac,
                 MXMDVertexDescriptor *skinDesc);
  void ApplyMorph(MXMDMorphTargets::Ptr &morphs, INode *node, Mesh *mesh,
                  MXMDMeshObject::Ptr &msh, MXMDModel::Ptr &mdl,
                  MXMDGeomBuffers::Ptr &buff);

  int LoadARC(const TCHAR *name, BOOL suppressPrompts, bool subLoad = false);
  int LoadSKL(const TCHAR *name, BOOL suppressPrompts, bool subLoad = false);
  int LoadMOT(const TCHAR *name, BOOL suppressPrompts, bool subLoad = false);
  int LoadANM(const TCHAR *name, BOOL suppressPrompts, bool subLoad = false);
  int LoadMXMD(const TCHAR *name, ImpInterface *importerInt, Interface *ip,
               BOOL suppressPrompts);
};

static class : public ClassDesc2 {
public:
  virtual int IsPublic() { return TRUE; }
  virtual void *Create(BOOL) { return new XenoImp(); }
  virtual const TCHAR *ClassName() { return _className; }
  virtual SClass_ID SuperClassID() { return SCENE_IMPORT_CLASS_ID; }
  virtual Class_ID ClassID() { return XenoImp_CLASS_ID; }
  virtual const TCHAR *Category() { return NULL; }
  virtual const TCHAR *InternalName() {
    return _className;
  } // returns fixed parsable name (scripter-visible name)
  virtual HINSTANCE HInstance() {
    return hInstance;
  } // returns owning module handle
} xenoImpDesc;

ClassDesc2 *GetXenoImpDesc() { return &xenoImpDesc; }

//--- ApexImp -------------------------------------------------------
XenoImp::XenoImp() {}

XenoImp::~XenoImp() {}

int XenoImp::ExtCount() { return 6; }

const TCHAR *XenoImp::Ext(int n) {
  switch (n) {
  case 1:
    return _T("camdo");
  case 2:
    return _T("arc");
  case 3:
    return _T("skl");
  case 4:
    return _T("mot");
  case 5:
    return _T("anm");
  default:
    break;
  }

  return _T("wimdo");
}

const TCHAR *XenoImp::LongDesc() { return _T("Xenoblade Inport"); }

const TCHAR *XenoImp::ShortDesc() { return _T("Xenoblade Import"); }

const TCHAR *XenoImp::AuthorName() { return _T(XenoMax_AUTHOR); }

const TCHAR *XenoImp::CopyrightMessage() { return _T(XenoMax_COPYRIGHT); }

const TCHAR *XenoImp::OtherMessage1() { return _T(""); }

const TCHAR *XenoImp::OtherMessage2() { return _T(""); }

unsigned int XenoImp::Version() { return XENOMAX_VERSIONINT; }

void XenoImp::ShowAbout(HWND hWnd) { ShowAboutDLG(hWnd); }

void XenoImp::LoadSkeleton(BCSKEL *skel) {
  BCSKEL::BoneData *boneData = skel->boneData.ptr;

  std::vector<INode *> nodes;

  for (int b = 0; b < boneData->boneLinks.count; b++) {
    const char *_boneName = boneData->boneNames.data[b].name;
    TSTRING boneName = esString(_boneName);
    INode *node = GetCOREInterface()->GetINodeByName(boneName.c_str());

    if (!node) {
      Object *obj = static_cast<Object *>(
          CreateInstance(HELPER_CLASS_ID, Class_ID(DUMMY_CLASS_ID, 0)));
      node = GetCOREInterface()->CreateObjectNode(obj);
      node->ShowBone(2);
      node->SetWireColor(0x80ff);
    }

    BCSKEL::BoneTransform &boneTM = boneData->boneTransforms.data[b];
    Matrix3 nodeTM = {};
    nodeTM.SetRotate(
        reinterpret_cast<const Quat &>(boneTM.rotation).Conjugate());
    nodeTM.SetTrans(reinterpret_cast<const Point3 &>(boneTM.position) *
                    IDC_EDIT_SCALE_value);
    nodeTM.Scale(reinterpret_cast<const Point3 &>(boneTM.scale));

    short parentID = boneData->boneLinks.data[b];
    if (parentID > -1) {
      nodes[parentID]->AttachChild(node);
      nodeTM *= nodes[parentID]->GetNodeTM(0);
    } else
      nodeTM *= corMat;

    node->SetNodeTM(0, nodeTM);
    node->SetName(ToBoneName(boneName));
    node->SetUserPropInt(_T("XenoBone"), b);
    nodes.push_back(node);
  }
}

static class : public ITreeEnumProc {
  const MSTR boneNameHint = _T("XenoBone");

public:
  std::vector<INode *> bones;

  void RescanBones() {
    bones.clear();
    GetCOREInterface7()->GetScene()->EnumTree(this);
  }

  INode *LookupNode(int ID) {
    for (auto &b : bones)
      if (b->UserPropExists(boneNameHint)) {
        int _ID;
        b->GetUserPropInt(boneNameHint, _ID);

        if (_ID == ID)
          return b;
      }

    return nullptr;
  }

  int callback(INode *node) {
    if (node->UserPropExists(boneNameHint))
      bones.push_back(node);

    return TREE_CONTINUE;
  }
} iBoneScanner;

void XenoImp::LoadAnimation(BCANIM *anim) {
  iBoneScanner.RescanBones();
  TimeValue numTicks = SecToTicks(anim->frameTime * anim->frameCount);
  TimeValue ticksPerFrame = GetTicksPerFrame();
  TimeValue overlappingTicks = numTicks % ticksPerFrame;

  if (overlappingTicks > (ticksPerFrame / 2))
    numTicks += ticksPerFrame - overlappingTicks;
  else
    numTicks -= overlappingTicks;

  Interval aniRange(0, numTicks - ticksPerFrame);
  GetCOREInterface()->SetAnimRange(aniRange);

  std::vector<float> frameTimes;

  for (TimeValue v = 0; v <= aniRange.End(); v += GetTicksPerFrame())
    frameTimes.push_back(TicksToSec(v));

  const int numAniBones = anim->animData->boneCount;

  for (int a = 0; a < numAniBones; a++) {
    const short boneID = anim->animData->boneTableOffset[a];

    if (boneID < 0)
      continue;

    INode *foundNode = iBoneScanner.LookupNode(a);

    if (!foundNode) {
      printwarning("[Xeno] Couldn't find XenoBone: ", << a);
      continue;
    }

    Control *cnt = foundNode->GetTMController();

    if (cnt->GetPositionController()->ClassID() !=
        Class_ID(LININTERP_POSITION_CLASS_ID, 0))
      cnt->SetPositionController((Control *)CreateInstance(
          CTRL_POSITION_CLASS_ID, Class_ID(LININTERP_POSITION_CLASS_ID, 0)));

    if (cnt->GetRotationController()->ClassID() !=
        Class_ID(LININTERP_ROTATION_CLASS_ID, 0))
      cnt->SetRotationController((Control *)CreateInstance(
          CTRL_ROTATION_CLASS_ID, Class_ID(LININTERP_ROTATION_CLASS_ID, 0)));

    if (cnt->GetScaleController()->ClassID() !=
        Class_ID(LININTERP_SCALE_CLASS_ID, 0))
      cnt->SetScaleController((Control *)CreateInstance(
          CTRL_SCALE_CLASS_ID, Class_ID(LININTERP_SCALE_CLASS_ID, 0)));

    SuspendAnimate();
    AnimateOn();

    cnt->AddNewKey(-ticksPerFrame, 0);

    for (auto &t : frameTimes) {
      BCANIM::TransformFrame evalTransform;

      anim->tracks.data[boneID].GetTransform(t, evalTransform, anim);

      Matrix3 cMat;
      Quat &rots = reinterpret_cast<Quat &>(evalTransform.rotation);
      cMat.SetRotate(rots.Conjugate());
      cMat.SetTrans(reinterpret_cast<Point3 &>(evalTransform.position) *
                    IDC_EDIT_SCALE_value);
      cMat.Scale(reinterpret_cast<Point3 &>(evalTransform.scale));

      if (!flags[IDC_CH_GLOBAL_FRAMES_checked]) {
        if (foundNode->GetParentNode()->IsRootNode())
          cMat *= corMat;

        SetXFormPacket packet(cMat);
        cnt->SetValue(SecToTicks(t), &packet);
      } else {
        foundNode->SetNodeTM(SecToTicks(t), cMat * corMat);
      }
    }

    AnimateOff();

    Control *rotControl = (Control *)CreateInstance(
        CTRL_ROTATION_CLASS_ID, Class_ID(HYBRIDINTERP_ROTATION_CLASS_ID, 0));
    rotControl->Copy(cnt->GetRotationController());
    cnt->GetRotationController()->Copy(rotControl);
  }
}

int XenoImp::LoadTextures(MXMD *model) {
  MXMDTextures::Ptr textures = model->GetTextures();
  MXMDExternalTextures::Ptr exTextures = model->GetExternalTextures();

  if (textures) {
    const int numTextures = textures->GetNumTextures();

    for (int t = 0; t < numTextures; t++) {
      const char *_texName = textures->GetTextureName(t);
      TSTRING texName = esStringConvert<TCHAR>(_texName);

      BitmapTex *maxBitmap = NewDefaultBitmapTex();
      maxBitmap->SetName(texName.c_str());
      texmaps.push_back(maxBitmap);
    }

    return 0;
  }

  if (exTextures) {
    const int numTextures = exTextures->GetNumTextures();

    for (int t = 0; t < numTextures; t++) {
      const int containerID = exTextures->GetContainerID(t);
      const int textureID = exTextures->GetExTextureID(t);

      TSTRING texName;

      if (containerID > -1) {
        texName.append(ToTSTRING(containerID));
        texName.push_back('/');
      }

      if (textureID < 1000)
        texName.push_back('0');
      if (textureID < 100)
        texName.push_back('0');
      if (textureID < 10)
        texName.push_back('0');

      texName.append(ToTSTRING(textureID));

      BitmapTex *maxBitmap = NewDefaultBitmapTex();
      maxBitmap->SetName(texName.c_str());
      texmaps.push_back(maxBitmap);
    }

    return 1;
  }

  return -1;
}

void XenoImp::LoadMaterials(MXMD *model) {
  MXMDMaterials::Ptr mats = model->GetMaterials();

  if (!mats)
    return;

  const int numMats = mats->GetNumMaterials();

  for (int m = 0; m < numMats; m++) {
    MXMDMaterial::Ptr cMat = mats->GetMaterial(m);
    const char *matName = cMat->GetName();
    StdMat *stdMat = NewDefaultStdMat();

    stdMat->SetName(esStringConvert<TCHAR>(matName).c_str());

    const int numTextures = cMat->GetNumTextures();
    CompositeTex cpTex;

    for (int t = 0; t < numTextures; t++) {
      const int texID = cMat->GetTextureIndex(t);

      if (texID >= texmaps.size())
        continue;

      BitmapTex *maxBitmap = texmaps[texID];
      TSTRING texName = maxBitmap->GetName();

      if (texName.find(_T("_SPM")) != texName.npos) {
        if (stdMat->GetSubTexmap(ID_SP)) {
          cpTex.AddLayer();
          CompositeTex::Layer clay = cpTex.GetLayer(cpTex.NumLayers() - 1);
          clay.Map(maxBitmap);
        } else
          stdMat->SetSubTexmap(ID_SP, maxBitmap);
      } else if (texName.find(_T("_GLO")) != texName.npos) {
        if (stdMat->GetSubTexmap(ID_SI)) {
          cpTex.AddLayer();
          CompositeTex::Layer clay = cpTex.GetLayer(cpTex.NumLayers() - 1);
          clay.Map(maxBitmap);
        } else
          stdMat->SetSubTexmap(ID_SI, maxBitmap);
      } else if (texName.find(_T("_RFM")) != texName.npos) {
        if (stdMat->GetSubTexmap(ID_SS)) {
          cpTex.AddLayer();
          CompositeTex::Layer clay = cpTex.GetLayer(cpTex.NumLayers() - 1);
          clay.Map(maxBitmap);
        } else
          stdMat->SetSubTexmap(ID_SS, maxBitmap);
      } else if (texName.find(_T("_NRM")) != texName.npos) {
        if (stdMat->GetSubTexmap(ID_SS)) {
          cpTex.AddLayer();
          CompositeTex::Layer clay = cpTex.GetLayer(cpTex.NumLayers() - 1);
          clay.Map(maxBitmap);
        } else
          stdMat->SetSubTexmap(ID_SS, NormalBump(maxBitmap));
      } else if (texName.find(_T("_COL")) != texName.npos) {
        if (stdMat->GetSubTexmap(ID_DI)) {
          cpTex.AddLayer();
          CompositeTex::Layer clay = cpTex.GetLayer(cpTex.NumLayers() - 1);
          clay.Map(maxBitmap);
        } else
          stdMat->SetSubTexmap(ID_DI, maxBitmap);
      } else {
        cpTex.AddLayer();
        CompositeTex::Layer clay = cpTex.GetLayer(cpTex.NumLayers() - 1);
        clay.Map(maxBitmap);
      }

      if (cpTex.NumLayers() > 1)
        stdMat->SetSubTexmap(ID_AM, cpTex);
    }

    outMats.push_back(stdMat);
  }
}

INodeTab XenoImp::LoadMeshes(MXMD *model, MXMDModel::Ptr &mdl, int curGroup) {
  ILayerManager *manager = GetCOREInterface13()->GetLayerManager();
  TSTRING assName(_T("Group"));
  INodeTab outNodes;
  MXMDGeomBuffers::Ptr geom = model->GetGeometry(curGroup);

  if (!geom)
    return {};

  MXMDMeshGroup::Ptr group = mdl->GetMeshGroup(curGroup);

  MSTR curAssName = assName.c_str();
  curAssName.append(ToTSTRING(curGroup).c_str());

  ILayer *currLayer = manager->GetLayer(curAssName);

  if (!currLayer)
    currLayer = manager->CreateLayer(curAssName);

  int currentMesh = 0;
  const int numMeshes = group->GetNumMeshObjects();
  outNodes.Resize(numMeshes);

  for (int m = 0; m < numMeshes; m++) {
    MXMDMeshObject::Ptr mObj = group->GetMeshObject(m);

    const int meshFacesID = mObj->GetUVFacesID();
    MXMDFaceBuffer::Ptr fBuffer = geom->GetFaceBuffer(meshFacesID);
    MXMDVertexBuffer::Ptr vBuffer = geom->GetVertexBuffer(mObj->GetBufferID());
    const int numVerts = vBuffer->NumVertices();
    const int numFaces = fBuffer->GetNumIndices() / 3;
    TriObject *obj = CreateNewTriObject();
    Mesh *msh = &obj->GetMesh();
    msh->setNumVerts(numVerts);
    msh->setNumFaces(numFaces);

    const USVector *fBuff = fBuffer->GetBuffer();
    MXMDVertexBuffer::DescriptorCollection descs = vBuffer->GetDescriptors();
    INodeSuffixer suff;
    int currentMap = 1;
    MXMDVertexDescriptor *skinDesc = nullptr;

    for (auto &d : descs)
      switch (d->Type()) {
      case MXMD_POSITION: {
        for (int v = 0; v < numVerts; v++) {
          Point3 temp;
          d->Evaluate(v, &temp);
          temp *= IDC_EDIT_SCALE_value;
          msh->setVert(v, corMat.VectorTransform(temp));
        }
        break;
      }
      case MXMD_UV1:
      case MXMD_UV2:
      case MXMD_UV3: {
        msh->setMapSupport(currentMap, 1);
        msh->setNumMapVerts(currentMap, numVerts);
        msh->setNumMapFaces(currentMap, numFaces);
        suff.AddChannel(currentMap);

        for (int v = 0; v < numVerts; v++) {
          Vector2 temp;
          d->Evaluate(v, &temp);
          msh->Map(currentMap).tv[v] = {temp.X, 1.f - temp.Y, 0.f};
        }

        currentMap++;
        break;
      }
      case MXMD_NORMAL:
      case MXMD_NORMAL2:
      case MXMD_NORMAL32: {
        suff.UseNormals();
        MeshNormalSpec *normalSpec;
        msh->SpecifyNormals();
        normalSpec = msh->GetSpecifiedNormals();
        normalSpec->ClearNormals();
        normalSpec->SetNumNormals(numVerts);
        normalSpec->SetNumFaces(numFaces);

        for (int v = 0; v < numVerts; v++) {
          Point3 temp;
          d->Evaluate(v, &temp);
          normalSpec->Normal(v) = corMat.VectorTransform(temp);
          normalSpec->SetNormalExplicit(v, true);
        }

        for (int f = 0; f < numFaces; f++) {
          MeshNormalFace &normalFace = normalSpec->Face(f);
          const USVector &tmp = fBuff[f];
          normalFace.SpecifyAll();
          normalFace.SetNormalID(0, tmp.X);
          normalFace.SetNormalID(1, tmp.Y);
          normalFace.SetNormalID(2, tmp.Z);
        }
        break;
      }
      case MXMD_VERTEXCOLOR: {
        msh->setMapSupport(-2, 1);
        msh->setNumMapVerts(-2, numVerts);
        msh->setNumMapFaces(-2, numFaces);
        msh->setMapSupport(0, 1);
        msh->setNumMapVerts(0, numVerts);
        msh->setNumMapFaces(0, numFaces);
        suff.AddChannel(0);
        suff.AddChannel(-2);

        for (int v = 0; v < numVerts; v++) {
          Vector4 temp;
          d->Evaluate(v, &temp);
          msh->Map(0).tv[v] = reinterpret_cast<Point3 &>(temp);
          msh->Map(-2).tv[v] = {temp.W, temp.W, temp.W};
        }
        break;
      }
      case MXMD_WEIGHTID:
        skinDesc = d.get();
        break;
      default:
        break;
      }

    MXMDMorphTargets::Ptr morphs =
        geom->GetVertexBufferMorphTargets(mObj->GetBufferID());

    if (morphs) {
      suff.UseMorph();
      MXMDVertexBuffer::DescriptorCollection morphDescs =
          morphs->GetBaseMorph();

      for (auto &d : morphDescs)
        switch (d->Type()) {
        case MXMD_POSITION: {
          for (int v = 0; v < numVerts; v++) {
            Point3 temp;
            d->Evaluate(v, &temp);
            temp *= IDC_EDIT_SCALE_value;
            msh->setVert(v, corMat.VectorTransform(temp));
          }
          break;
        }

        case MXMD_NORMALMORPH: {
          suff.UseNormals();
          MeshNormalSpec *normalSpec;
          msh->SpecifyNormals();
          normalSpec = msh->GetSpecifiedNormals();
          normalSpec->ClearNormals();
          normalSpec->SetNumNormals(numVerts);
          normalSpec->SetNumFaces(numFaces);

          for (int v = 0; v < numVerts; v++) {
            Vector4 temp;
            d->Evaluate(v, &temp);
            normalSpec->Normal(v) =
                corMat.VectorTransform(reinterpret_cast<Point3 &>(temp));
            normalSpec->SetNormalExplicit(v, true);
          }

          for (int f = 0; f < numFaces; f++) {
            MeshNormalFace &normalFace = normalSpec->Face(f);
            const USVector &tmp = fBuff[f];
            normalFace.SpecifyAll();
            normalFace.SetNormalID(0, tmp.X);
            normalFace.SetNormalID(1, tmp.Y);
            normalFace.SetNormalID(2, tmp.Z);
          }
          break;
        }
        }
    }

    for (int f = 0; f < numFaces; f++) {
      Face &face = msh->faces[f];
      face.setEdgeVisFlags(1, 1, 1);
      const USVector &tmp = fBuff[f];
      face.v[0] = tmp.X;
      face.v[1] = tmp.Y;
      face.v[2] = tmp.Z;

      for (int &i : suff)
        msh->Map(i).tf[f].setTVerts(tmp.X, tmp.Y, tmp.Z);
    }

    msh->DeleteIsoVerts();
    msh->DeleteIsoMapVerts();
    msh->InvalidateGeomCache();
    msh->InvalidateTopologyCache();

    INode *nde = GetCOREInterface()->CreateObjectNode(obj);
    int gibid = mObj->GetGibID();
    TSTRING nodeName;

    if (gibid) {
      nodeName.append(_T("Part"));
      nodeName.append(ToTSTRING(gibid));
    } else {
      nodeName.append(_T("Object"));
      nodeName.append(ToTSTRING(currentMesh));
      currentMesh++;
    }

    int LOD = mObj->GetLODID();

    if (LOD > 0) {
      MSTR curAssName = assName.c_str();
      curAssName.append(ToTSTRING(curGroup).c_str());
      curAssName.append(_T("_LOD")).append(ToTSTRING(LOD).c_str());

      ILayer *currLODLayer = manager->GetLayer(curAssName);

      if (!currLODLayer)
        currLODLayer = manager->CreateLayer(curAssName);

      currLODLayer->AddToLayer(nde);
    } else
      currLayer->AddToLayer(nde);

    suff.node = nde;

    if (morphs)
      ApplyMorph(morphs, nde, msh, mObj, mdl, geom);

    if (skinDesc)
      ApplySkin(geom, mObj, suff, msh->faces, skinDesc);

    if (flags[IDC_CH_DEBUGNAME_checked])
      nodeName.append(suff.Generate());

    nde->SetName(ToBoneName(nodeName));

    const int matID = mObj->GetMaterialID();

    if (matID < outMats.size())
      nde->SetMtl(outMats[matID]);

    outNodes.AppendNode(nde);
  }

  return outNodes;
}

void XenoImp::LoadModels(MXMD *model) {
  MXMDModel::Ptr mdl = model->GetModel();

  if (!mdl)
    return;

  LoadModelPose(mdl);

  const int numMeshGroups = mdl->GetNumMeshGroups();

  for (int g = 0; g < numMeshGroups; g++)
    LoadMeshes(model, mdl, g);
}

void XenoImp::LoadModelPose(MXMDModel::Ptr &model) {
  const int numBones = model->GetNumSkinBones();

  for (int b = 0; b < numBones; b++) {
    MXMDBone::Ptr cBone = model->GetSkinBone(b);

    const char *_boneName = cBone->GetName();
    TSTRING boneName = esString(_boneName);
    INode *node = GetCOREInterface()->GetINodeByName(boneName.c_str());

    if (!node) {
      Object *obj = static_cast<Object *>(
          CreateInstance(HELPER_CLASS_ID, Class_ID(DUMMY_CLASS_ID, 0)));
      node = GetCOREInterface()->CreateObjectNode(obj);
      node->ShowBone(2);
      node->SetWireColor(0x80ff);
      node->SetName(ToBoneName(boneName));

      Matrix3 nodeTM = {};
      const MXMDTransformMatrix *bneMat = cBone->GetAbsTransform();

      nodeTM.SetRow(0, reinterpret_cast<const Point3 &>(bneMat->m[0]));
      nodeTM.SetRow(1, reinterpret_cast<const Point3 &>(bneMat->m[1]));
      nodeTM.SetRow(2, reinterpret_cast<const Point3 &>(bneMat->m[2]));
      nodeTM.SetRow(3, reinterpret_cast<const Point3 &>(bneMat->m[3]) *
                           IDC_EDIT_SCALE_value);
      nodeTM.Invert();
      node->SetNodeTM(0, nodeTM * corMat);
    }

    remapNodes.push_back(node);
  }

  for (int b = 0; b < numBones; b++) {
    MXMDBone::Ptr cBone = model->GetSkinBone(b);
    int parentID = cBone->GetParentID();

    if (parentID > -1) {
      MXMDBone::Ptr pBone = model->GetBone(parentID);
      const char *_pboneName = pBone->GetName();
      TSTRING pBoneName = esString(_pboneName);
      INode *pNode = GetCOREInterface()->GetINodeByName(pBoneName.c_str());

      if (pNode)
        pNode->AttachChild(remapNodes[b]);
    }
  }
}

void XenoImp::ApplySkin(MXMDGeomBuffers::Ptr &buff, MXMDMeshObject::Ptr &mesh,
                        INodeSuffixer &nde, Face *mfac,
                        MXMDVertexDescriptor *skinDesc) {
  if (!remapNodes.size())
    return;
  else if (remapNodes.size() == 1) {
    remapNodes[0]->AttachChild(nde);
    return;
  }

  const int skindesc =
      ((mesh->GetLODID() << 8) & 0xff00) | mesh->GetSkinDesc() & 0xff;

  MXMDGeomVertexWeightBuffer::Ptr wtBuff = buff->GetWeightsBuffer(skindesc);

  if (!buff)
    return;

  nde.UseSkin();

  Modifier *cmod = (Modifier *)GetCOREInterface()->CreateInstance(OSM_CLASS_ID,
                                                                  SKIN_CLASSID);
  GetCOREInterface7()->AddModifier(*nde, *cmod);
  ISkinImportData *cskin =
      (ISkinImportData *)cmod->GetInterface(I_SKINIMPORTDATA);

  for (auto &r : remapNodes)
    cskin->AddBoneEx(r, false);

  static_cast<INode *>(nde)->EvalWorldState(0);

  const int meshFacesID = mesh->GetUVFacesID();
  MXMDVertexBuffer::Ptr vBuffer = buff->GetVertexBuffer(mesh->GetBufferID());
  MXMDFaceBuffer::Ptr fBuffer = buff->GetFaceBuffer(meshFacesID);
  const int numVerts = vBuffer->NumVertices();
  const int numFaces = fBuffer->GetNumIndices() / 3;
  const USVector *fBuff = fBuffer->GetBuffer();
  BitArray btarr(numVerts);

  for (int f = 0; f < numFaces; f++) {
    const USVector &cFaceBegin = fBuff[f];
    for (int s = 0; s < 3; s++) {
      const int &cfseg = mfac[f].v[s];
      if (!btarr[cfseg]) {
        ushort vtid = 0;
        skinDesc->Evaluate(cFaceBegin[s], &vtid);

        MXMDVertexWeight cWtOut = wtBuff->GetVertexWeight(vtid);
        Tab<INode *> cbn;
        Tab<float> cwt;
        cbn.SetCount(4);
        cwt.SetCount(4);
        for (int u = 0; u < 4; u++) {
          cbn[u] = remapNodes[cWtOut.boneids[u]];
          cwt[u] = cWtOut.weights[u];
        }
        cskin->AddWeights(nde, cfseg, cbn, cwt);

        btarr.Set(cfseg);
      }
    }
  }
}

void XenoImp::ApplyMorph(MXMDMorphTargets::Ptr &morphs, INode *node, Mesh *mesh,
                         MXMDMeshObject::Ptr &msh, MXMDModel::Ptr &mdl,
                         MXMDGeomBuffers::Ptr &buff) {
  Modifier *cmod = (Modifier *)GetCOREInterface()->CreateInstance(OSM_CLASS_ID,
                                                                  MR3_CLASS_ID);
  GetCOREInterface7()->AddModifier(*node, *cmod);

  MaxMorphModifier morpher = {};
  morpher.Init(cmod);

  const int targetcount = morphs->GetNumMorphs();
  const int meshFacesID = msh->GetUVFacesID();
  MXMDVertexBuffer::Ptr vBuffer = buff->GetVertexBuffer(msh->GetBufferID());
  MXMDFaceBuffer::Ptr fBuffer = buff->GetFaceBuffer(meshFacesID);
  const int numVerts = vBuffer->NumVertices();
  const int numFaces = fBuffer->GetNumIndices() / 3;
  const USVector *fBuff = fBuffer->GetBuffer();
  BitArray btarr(numVerts);
  int currentChannel = 0;

  for (int m = 0; m < targetcount; m++) {
    MXMDVertexBuffer::DescriptorCollection morph = morphs->GetDeltaMorph(m);

    MaxMorphChannel &chan = morpher.GetMorphChannel(currentChannel);
    chan.Reset(true, true, mesh->numVerts);

    TSTRING morphName = esString(mdl->GetMorphName(morphs->GetMorphNameID(m)));

    if (!morphName.size())
      morphName = _T("Morph ") + ToTSTRING(currentChannel);

    chan.SetName(morphName.c_str());

    for (int v = 0; v < mesh->numVerts; v++)
      chan.SetMorphPointDelta(v, Point3{0, 0, 0});

    btarr.ClearAll();

    std::vector<int> IDS;

    for (auto &m : morph)
      switch (m->Type()) {
      case MXMD_MORPHVERTEXID: {
        const int numItems = m->Size();
        IDS.resize(numItems);

        for (int i = 0; i < numItems; i++)
          m->Evaluate(i, &IDS[i]);

        break;
      }
      case MXMD_POSITION: {
        int currentNumberOfChannelVertices = 0;

        for (int f = 0; f < numFaces; f++) {
          const USVector &cFaceBegin = fBuff[f];

          for (int s = 0; s < 3; s++) {
            const int &cfseg = mesh->faces[f].v[s];

            if (!btarr[cfseg]) {
              int currentMorphVertID = 0;

              for (auto &cid : IDS) {
                if (cFaceBegin[s] == cid) {
                  Point3 pos;
                  m->Evaluate(currentMorphVertID, &pos);
                  chan.SetMorphPointDelta(cfseg, corMat.VectorTransform(pos));
                  currentNumberOfChannelVertices++;
                }
                currentMorphVertID++;
              }
              btarr.Set(cfseg);
            }
          }
        }

        if (currentNumberOfChannelVertices)
          currentChannel++;

        break;
      }
      }
  }
}

int XenoImp::LoadInstances(MXMD *model) {
  MXMDModel::Ptr mdl = model->GetModel();
  MXMDInstances::Ptr insts = model->GetInstances();

  if (!mdl || !insts)
    return 1;

  const int numInstances = insts->GetNumInstances();
  const int numGroups = mdl->GetNumMeshGroups();
  std::vector<INodeTab> instances(numGroups);

  for (int i = 0; i < numInstances; i++) {
    const MXMDTransformMatrix *mtx = insts->GetTransform(i);

    int groupBegin = insts->GetStartingGroup(i);
    const int groupEnd = groupBegin + insts->GetNumGroups(i);

    for (groupBegin; groupBegin < groupEnd; groupBegin++) {
      int meshGroupID = insts->GetMeshGroup(groupBegin);

      if (meshGroupID > numGroups || meshGroupID < 0)
        continue;

      INodeTab meshes;

      if (!instances[meshGroupID].Count()) {
        meshes = LoadMeshes(model, mdl, meshGroupID);
        instances[meshGroupID] = meshes;
      } else
        GetCOREInterface()->CloneNodes(instances[meshGroupID], Point3(), false,
                                       NODE_INSTANCE, nullptr, &meshes);

      for (int m = 0; m < meshes.Count(); m++) {
        INode *node = meshes[m];
        Matrix3 nodeTM = {};
        nodeTM.SetRow(0, reinterpret_cast<const Point3 &>(mtx->m[0]));
        nodeTM.SetRow(1, reinterpret_cast<const Point3 &>(mtx->m[1]));
        nodeTM.SetRow(2, reinterpret_cast<const Point3 &>(mtx->m[2]));
        nodeTM.SetRow(3, reinterpret_cast<const Point3 &>(mtx->m[3]) *
                             IDC_EDIT_SCALE_value);
        Matrix3 nodeTM2 = corMat;
        nodeTM2.Invert();
        nodeTM2 *= nodeTM;
        node->SetNodeTM(0, nodeTM2 * corMat);
      }
    }
  }

  return 0;
}

int XenoImp::LoadSKL(const TCHAR *filename, BOOL suppressPrompts,
                     bool subLoad) {
  BC sklFile;
  int loadResult = sklFile.Load(filename, !subLoad);

  if (loadResult)
    return loadResult;

  BCSKEL *skl = sklFile.GetClass<BCSKEL>();

  if (!skl)
    return -1;

  if (!suppressPrompts)
    if (!SpawnANIDialog())
      return 0;

  LoadSkeleton(skl);

  return 0;
}

int XenoImp::LoadARC(const TCHAR *filename, BOOL suppressPrompts,
                     bool subLoad) {
  SAR arcFile;
  int loadResult = arcFile.Load(filename, !subLoad);

  if (loadResult)
    return loadResult;

  int ext = arcFile.FileIndexFromExtension(".skl");

  if (ext < 0)
    return ext;

  BC sklFile;
  loadResult = sklFile.Link(arcFile.GetFile(ext));

  if (loadResult)
    return loadResult;

  BCSKEL *skl = sklFile.GetClass<BCSKEL>();

  if (!skl)
    return -1;

  if (!suppressPrompts)
    if (!SpawnANIDialog())
      return 0;

  LoadSkeleton(skl);

  return 0;
}

int XenoImp::LoadANM(const TCHAR *filename, BOOL suppressPrompts,
                     bool subLoad) {
  BC anmFile;
  int loadResult = anmFile.Load(filename, !subLoad);

  if (loadResult)
    return loadResult;

  BCANIM *anm = anmFile.GetClass<BCANIM>();

  if (!anm)
    return -1;

  if (!suppressPrompts)
    if (!SpawnANIDialog())
      return 0;

  LoadAnimation(anm);

  return 0;
}

int XenoImp::LoadMOT(const TCHAR *filename, BOOL suppressPrompts,
                     bool subLoad) {
  SAR arcFile;
  int loadResult = arcFile.Load(filename, !subLoad);

  if (loadResult)
    return loadResult;

  for (int f = 0; f < arcFile.NumFiles(); f++) {
    AFileInfo fleInfo(arcFile.GetFileName(f));

    if (fleInfo.GetExtension().compare(".anm"))
      continue;

    BC anmFile;

    if (anmFile.Link(arcFile.GetFile(f)))
      continue;

    BCANIM *anm = anmFile.GetClass<BCANIM>();

    if (!anm)
      continue;

    XenoImp::MotionPair mtPair;
    mtPair.name = fleInfo.GetFileName().c_str();
    mtPair.ID = f;

    motions.push_back(mtPair);
  }

  if (!suppressPrompts)
    if (!SpawnANIDialog())
      return 0;

  BC anmFile;
  anmFile.Link(arcFile.GetFile(motions[IDC_CB_MOTIONINDEX_index].ID));

  BCANIM *anm = anmFile.GetClass<BCANIM>();

  LoadAnimation(anm);

  return 0;
}

int XenoImp::LoadMXMD(const TCHAR *filename, ImpInterface *importerInt,
                      Interface *ip, BOOL suppressPrompts) {

  TFileInfo fleInfo(filename);
  TSTRING baseFilePath = fleInfo.GetPath() + fleInfo.GetFileName();
  TSTRING arcFilepath = baseFilePath + _T(".arc");

  if (!suppressPrompts)
    if (!SpawnMXMDDialog())
      return 0;

  if (DoesFileExist(arcFilepath.c_str(), false)) {
    LoadARC(arcFilepath.c_str(), TRUE, true);
  } else {
    TSTRING hkcfgpath =
        IPathConfigMgr::GetPathConfigMgr()->GetDir(APP_PLUGCFG_DIR);
    hkcfgpath.append(_T("\\HavokImpSettings.ini"));
    WriteText(_T("HK_PRESETS_HEADER"), _T("Xenoblade"), hkcfgpath.c_str(),
              _T("currentPresetName"));

    SceneImport *hkImportInterface = static_cast<SceneImport *>(
        CreateInstance(SCENE_IMPORT_CLASS_ID, HavokImport_CLASS_ID));

    if (hkImportInterface) {
      TSTRING sklFilePath = baseFilePath + _T("_ev_rig.hkt");

      if (!DoesFileExist(sklFilePath.c_str(), false))
        sklFilePath = baseFilePath + _T("_rig.hkt");

      if (DoesFileExist(sklFilePath.c_str(), false)) {
        hkImportInterface->DoImport(sklFilePath.c_str(), importerInt, ip, TRUE);
      }
    }
  }

  MXMD mainModel;

  if (mainModel.Load(filename))
    return 1;

  TSTRING folderPath = fleInfo.GetPath() + fleInfo.GetFileName() + _T("/");

  std::thread texThread;

  if (flags[IDC_CH_TEXTURES_checked])
    texThread = std::thread([&] {
      MXMDTextures::Ptr textures = mainModel.GetTextures();

      if (!textures)
        return;

      TextureConversionParams params;
      params.allowBC5ZChan = !flags[IDC_CH_BC5BCHAN_checked];
      params.uncompress = flags[IDC_CH_TOPNG_checked];

      _tmkdir(folderPath.c_str());

      textures->ExtractAllTextures(folderPath.c_str(), params);
    });

  int textureLocation = LoadTextures(&mainModel);
  LoadMaterials(&mainModel);

  if (LoadInstances(&mainModel))
    LoadModels(&mainModel);

  if (texThread.joinable())
    texThread.join();

  for (auto &t : texmaps) {
    const TCHAR *texName = t->GetName();
    TSTRING texFullPath;

    if (textureLocation) {
      texFullPath = fleInfo.GetPath();
      texFullPath.pop_back();

      TFileInfo tfle(texFullPath);

      texFullPath = tfle.GetPath() + _T("textures/") + texName;
    } else
      texFullPath = folderPath + texName;

    if (DoesFileExist((texFullPath + _T(".png")).c_str(), false))
      texFullPath.append(_T(".png"));
    else
      texFullPath.append(_T(".dds"));

    t->SetMapName(texFullPath.c_str());
  }

  return 0;
}

int XenoImp::DoImport(const TCHAR *filename, ImpInterface *importerInt,
                      Interface *ip, BOOL suppressPrompts) {
  char *oldLocale = setlocale(LC_NUMERIC, NULL);
  setlocale(LC_NUMERIC, "en-US");
  int result = FALSE;

  TFileInfo fleInfo(filename);
  TSTRING extension = fleInfo.GetExtension();

  if (!extension.compare(_T(".arc")))
    result = !LoadARC(filename, suppressPrompts);
  else if (!extension.compare(_T(".skl")))
    result = !LoadSKL(filename, suppressPrompts);
  else if (!extension.compare(_T(".mot")))
    result = !LoadMOT(filename, suppressPrompts);
  else if (!extension.compare(_T(".anm")))
    result = !LoadANM(filename, suppressPrompts);
  else
    result = !LoadMXMD(filename, importerInt, ip, suppressPrompts);

  setlocale(LC_NUMERIC, oldLocale);
  PrintOffThreadMessages();
  return result;
}
