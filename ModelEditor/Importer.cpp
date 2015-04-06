///////////
// This file is a part of the ATools project
// Some parts of code are the property of Microsoft, Qt or Aeonsoft
// The rest is released without license and without any warranty
///////////

#include "stdafx.h"
#include "Importer.h"
#include <AnimatedMesh.h>
#include <Object3D.h>
#include <Motion.h>

using namespace Assimp;

CImporter::CImporter(CAnimatedMesh* mesh)
	: m_mesh(mesh),
	m_scene(null),
	m_obj3D(null),
	m_externBones(false),
	m_frameCount(0),
	m_bones(null)
{
}

CImporter::~CImporter()
{
	if (!m_mesh->m_skeleton && !m_mesh->m_motion && m_obj3D)
	{
		if (!m_obj3D->m_motion)
			DeleteArray(m_bones);
	}
}

bool CImporter::Import(const string& filename)
{
	unsigned int flags = aiProcess_JoinIdenticalVertices;
	flags |= aiProcess_FlipUVs;
	flags |= aiProcess_Triangulate;
	flags |= aiProcess_GenSmoothNormals;
	flags |= aiProcess_LimitBoneWeights;
	flags |= aiProcess_ValidateDataStructure;
	flags |= aiProcess_ImproveCacheLocality;
	flags |= aiProcess_RemoveRedundantMaterials;
	flags |= aiProcess_FixInfacingNormals;
	flags |= aiProcess_FindInvalidData;
	flags |= aiProcess_OptimizeMeshes;
	flags |= aiProcess_SplitByBoneCount;
	flags |= aiProcess_TransformUVCoords;
	flags |= aiProcess_SortByPType;
	flags |= aiProcess_RemoveComponent;
	flags |= aiProcess_MakeLeftHanded;
	//flags |= aiProcess_FlipWindingOrder;
	//flags |= aiProcess_OptimizeGraph;

	Importer importer;
	if (!importer.ValidateFlags(flags))
	{
		qCritical("Import flags not supported");
		return false;
	}

	if (!importer.IsExtensionSupported(('.' % GetExtension(filename)).toLocal8Bit().constData()))
	{
		qCritical("This file format isn't supported");
		return false;
	}

	importer.SetPropertyInteger(AI_CONFIG_PP_LBW_MAX_WEIGHTS, 2);
	importer.SetPropertyInteger(AI_CONFIG_PP_SBBC_MAX_BONES, MAX_BONES);
	importer.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS, aiComponent_TANGENTS_AND_BITANGENTS
		| aiComponent_COLORS
		| aiComponent_TEXTURES
		| aiComponent_LIGHTS
		| aiComponent_CAMERAS);

	m_scene = (aiScene*)importer.ReadFile(filename.toLocal8Bit().constData(), flags);
	if (!m_scene)
	{
		qCritical(importer.GetErrorString());
		return false;
	}

	if (!m_scene->HasMeshes() || !m_scene->mRootNode || m_scene->mRootNode->mNumChildren == 0)
	{
		qCritical("No meshes in this scene");
		return false;
	}

	m_obj3D = m_mesh->m_elements[0].obj = new CObject3D(m_mesh->m_device);
	m_obj3D->m_ID = _getNewID();

	const string filenameToLower = QFileInfo(filename).fileName().toLower();

	if (filenameToLower.startsWith("mvr")
		|| filenameToLower.startsWith("part")
		|| filenameToLower.startsWith("item"))
		m_externBones = true;

	_importScene();

	if (!m_obj3D->InitDeviceObjects())
	{
		qCritical(("Can't init object3D device objects '" % filename % "'").toLocal8Bit().data());
		return false;
	}

	importer.FreeScene();
	return true;
}

void CImporter::_importScene()
{
	_scanNode(m_scene->mRootNode);

	if (m_boneNodes.size() > 0)
		_createBones();

	_createGMObjects();

	if (m_scene->HasAnimations())
		_createAnimations();

	if (m_obj3D->m_collObj.type != EGMT_ERROR)
		_preTransformVertices(&m_obj3D->m_collObj);

	_calculateBounds();
}

void CImporter::_calculateBounds()
{
	LODGroup* group = &m_obj3D->m_groups[0];

	bool normalObj = false;
	int i, j;

	for (i = 0; i <group->objectCount; i++)
	{
		if (!group->objects[i].physiqueVertices)
		{
			normalObj = true;
			break;
		}
	}

	D3DXMATRIX* updates = null;
	GMObject* obj = null;

	if (normalObj)
	{
		updates = new D3DXMATRIX[group->objectCount];
		for (i = 0; i < group->objectCount; i++)
			D3DXMatrixIdentity(&updates[i]);

		for (i = 0; i < group->objectCount; i++)
		{
			obj = &group->objects[i];

			if (obj->parentID == -1)
				updates[i] = obj->transform;
			else
			{
				if (obj->parentType == EGMT_BONE)
					updates[i] = obj->transform * m_bones[obj->parentID].TM;
				else
					updates[i] = obj->transform * updates[obj->parentID];
			}
		}
	}

	D3DXVECTOR3 bbMin(FLT_MAX, FLT_MAX, FLT_MAX);
	D3DXVECTOR3 bbMax(FLT_MIN, FLT_MIN, FLT_MIN);

	D3DXMATRIX mat;
	D3DXVECTOR3 vecs[2];
	D3DXVECTOR3 v;
	for (i = 0; i < group->objectCount; i++)
	{
		obj = &group->objects[i];

		if (obj->type == EGMT_SKIN)
			D3DXMatrixIdentity(&mat);
		else
			mat = updates[i];

		D3DXVec3TransformCoord(&vecs[0], &obj->bounds.Min, &mat);
		D3DXVec3TransformCoord(&vecs[1], &obj->bounds.Max, &mat);

		for (j = 0; j < 2; j++)
		{
			v = vecs[j];

			if (v.x < bbMin.x)
				bbMin.x = v.x;
			if (v.y < bbMin.y)
				bbMin.y = v.y;
			if (v.z < bbMin.z)
				bbMin.z = v.z;

			if (v.x > bbMax.x)
				bbMax.x = v.x;
			if (v.y > bbMax.y)
				bbMax.y = v.y;
			if (v.z > bbMax.z)
				bbMax.z = v.z;
		}
	}

	DeleteArray(updates);

	m_obj3D->m_bounds.Min = bbMin;
	m_obj3D->m_bounds.Max = bbMax;

	m_mesh->m_bounds.Min = D3DXVECTOR3(FLT_MAX, FLT_MAX, FLT_MAX);
	m_mesh->m_bounds.Max = D3DXVECTOR3(FLT_MIN, FLT_MIN, FLT_MIN);

	if (m_mesh->m_bounds.Min.x > bbMin.x) m_mesh->m_bounds.Min.x = bbMin.x;
	if (m_mesh->m_bounds.Min.y > bbMin.y) m_mesh->m_bounds.Min.y = bbMin.y;
	if (m_mesh->m_bounds.Min.z > bbMin.z) m_mesh->m_bounds.Min.z = bbMin.z;
	if (m_mesh->m_bounds.Max.x < bbMax.x) m_mesh->m_bounds.Max.x = bbMax.x;
	if (m_mesh->m_bounds.Max.y < bbMax.y) m_mesh->m_bounds.Max.y = bbMax.y;
	if (m_mesh->m_bounds.Max.z < bbMax.z) m_mesh->m_bounds.Max.z = bbMax.z;
}

void CImporter::_preTransformVertices(GMObject* obj)
{
	int i;
	if (obj->type == EGMT_SKIN)
	{
		SkinVertex* vertices = (SkinVertex*)obj->vertices;
		for (i = 0; i < obj->vertexCount; i++)
		{
			D3DXVec3TransformCoord(&vertices[i].p, &vertices[i].p, &obj->transform);
			D3DXVec3TransformCoord(&vertices[i].n, &vertices[i].n, &obj->transform);
			D3DXVec3Normalize(&vertices[i].n, &vertices[i].n);
		}
	}
	else
	{
		NormalVertex* vertices = (NormalVertex*)obj->vertices;
		for (i = 0; i < obj->vertexCount; i++)
		{
			D3DXVec3TransformCoord(&vertices[i].p, &vertices[i].p, &obj->transform);
			D3DXVec3TransformCoord(&vertices[i].n, &vertices[i].n, &obj->transform);
			D3DXVec3Normalize(&vertices[i].n, &vertices[i].n);
		}
	}

	for (i = 0; i < obj->vertexListCount; i++)
		D3DXVec3TransformCoord(&obj->vertexList[i], &obj->vertexList[i], &obj->transform);

	D3DXMatrixIdentity(&obj->transform);
}

void CImporter::_createAnimations()
{
	aiAnimation* anim = null;
	for (uint i = 0; i < m_scene->mNumAnimations; i++)
	{
		if (!anim)
			anim = m_scene->mAnimations[i];
		else
		{
			if (m_scene->mAnimations[i]->mDuration > anim->mDuration)
				anim = m_scene->mAnimations[i];
		}
	}

	if (!anim || !anim->mNumChannels)
		return;

	if (anim->mTicksPerSecond != 0.0)
		m_ticksPerSecond = anim->mTicksPerSecond;
	else
		m_ticksPerSecond = 1.0;
	m_frameCount = (int)((anim->mDuration / m_ticksPerSecond) * 30.0 + 1.5);

	if (!m_frameCount)
		return;

	QMap<int, TMAnimation*> bonesAni;
	bool found;
	aiNodeAnim* nodeAnim = null;
	int j;
	for (uint i = 0; i < anim->mNumChannels; i++)
	{
		nodeAnim = anim->mChannels[i];
		TMAnimation* frames = _createAnimation(nodeAnim);

		const string name(string(nodeAnim->mNodeName.C_Str()));
		auto it = m_mapObjects.find(string(nodeAnim->mNodeName.C_Str()));
		if (it != m_mapObjects.end())
		{
			it.value()->frames = frames;
			m_obj3D->m_frameCount = m_frameCount;
			continue;
		}

		found = false;
		for (j = 0; j < m_boneNodes.size(); j++)
		{
			if (name == m_bones[j].name)
			{
				bonesAni[j] = frames;
				found = true;
				break;
			}
		}

		if (!found)
			DeleteArray(frames);
	}

	m_mesh->m_frameCount = m_frameCount;

	if (bonesAni.size() > 0)
	{
		CMotion* motion = new CMotion();
		motion->m_ID = _getNewID();
		motion->m_boneCount = m_boneNodes.size();
		motion->m_bones = m_bones;
		motion->m_frameCount = m_frameCount;
		motion->m_attributes = new MotionAttribute[motion->m_frameCount];
		memset(motion->m_attributes, 0, sizeof(MotionAttribute) * motion->m_frameCount);
		motion->m_animations = new TMAnimation[bonesAni.size() * m_frameCount];
		memset(motion->m_animations, 0, sizeof(TMAnimation) * bonesAni.size() * m_frameCount);
		motion->m_frames = new BoneFrame[motion->m_boneCount];
		memset(motion->m_frames, 0, sizeof(BoneFrame) * motion->m_boneCount);

		TMAnimation* frames = motion->m_animations;
		for (int i = 0; i < motion->m_boneCount; i++)
		{
			if (bonesAni.find(i) != bonesAni.end())
			{
				motion->m_frames[i].frames = frames;
				memcpy(frames, bonesAni[i], sizeof(TMAnimation) * m_frameCount);
				frames += m_frameCount;
			}
			else
			{
				motion->m_frames[i].frames = null;
				motion->m_frames[i].TM = m_bones[i].localTM;
			}
		}

		if (m_externBones)
		{
			m_mesh->m_motion = motion;
			m_mesh->m_skeleton->m_bones = new Bone[m_mesh->m_skeleton->m_boneCount];
			memcpy(m_mesh->m_skeleton->m_bones, m_bones, sizeof(Bone) * m_mesh->m_skeleton->m_boneCount);
		}
		else
		{
			m_obj3D->m_motion = motion;
			m_obj3D->m_frameCount = m_frameCount;
		}
	}

	if (bonesAni.size() > 0)
	{
		for (auto it = bonesAni.begin(); it != bonesAni.end(); it++)
			DeleteArray(it.value());
	}

	if (m_obj3D->m_frameCount)
	{
		m_obj3D->m_attributes = new MotionAttribute[m_obj3D->m_frameCount];
		memset(m_obj3D->m_attributes, 0, sizeof(MotionAttribute) * m_obj3D->m_frameCount);
	}
}

TMAnimation* CImporter::_createAnimation(aiNodeAnim* anim)
{
	TMAnimation* frames = new TMAnimation[m_frameCount];
	memset(frames, 0, sizeof(TMAnimation) * m_frameCount);

	uint j, k;
	aiVectorKey* firstPos, *lastPos;
	double firstTime = 99999.0, lastTime = -1.0;
	for (j = 0; j < anim->mNumPositionKeys; j++)
	{
		if (anim->mPositionKeys[j].mTime < firstTime)
		{
			firstPos = &anim->mPositionKeys[j];
			firstTime = anim->mPositionKeys[j].mTime;
		}
		if (anim->mPositionKeys[j].mTime > lastTime)
		{
			lastPos = &anim->mPositionKeys[j];
			lastTime = anim->mPositionKeys[j].mTime;
		}
	}

	firstTime = 99999.0; lastTime = -1.0;
	aiQuatKey* firstRot, *lastRot;
	for (j = 0; j < anim->mNumRotationKeys; j++)
	{
		if (anim->mRotationKeys[j].mTime < firstTime)
		{
			firstRot = &anim->mRotationKeys[j];
			firstTime = anim->mRotationKeys[j].mTime;
		}
		if (anim->mRotationKeys[j].mTime > lastTime)
		{
			lastRot = &anim->mRotationKeys[j];
			lastTime = anim->mRotationKeys[j].mTime;
		}
	}

	D3DXVECTOR3 prevVec, nextVec, lerp;
	D3DXQUATERNION prevQuat, nextQuat, slerp;

	aiVectorKey* prevPos, *nextPos;
	aiQuatKey* prevRot, *nextRot;
	for (int i = 0; i < m_frameCount; i++)
	{
		const double time = ((double)i * m_ticksPerSecond * (1.0 / 30.0));
		prevPos = null;
		nextPos = null;
		firstTime = -1.0; lastTime = 99999.0;

		for (j = 0; j < anim->mNumPositionKeys; j++)
		{
			if (anim->mPositionKeys[j].mTime <= time
				&& anim->mPositionKeys[j].mTime > firstTime)
			{
				prevPos = &anim->mPositionKeys[j];
				firstTime = anim->mPositionKeys[j].mTime;
			}
			if (anim->mPositionKeys[j].mTime >= time
				&& anim->mPositionKeys[j].mTime < lastTime)
			{
				nextPos = &anim->mPositionKeys[j];
				lastTime = anim->mPositionKeys[j].mTime;
			}
		}

		if (!prevPos)
			prevPos = firstPos;
		if (!nextPos)
			nextPos = lastPos;

		prevRot = null;
		nextRot = null;
		firstTime = -1.0; lastTime = 99999.0;

		for (j = 0; j < anim->mNumRotationKeys; j++)
		{
			if (anim->mRotationKeys[j].mTime <= time
				&& anim->mRotationKeys[j].mTime > firstTime)
			{
				prevRot = &anim->mRotationKeys[j];
				firstTime = anim->mRotationKeys[j].mTime;
			}
			if (anim->mRotationKeys[j].mTime >= time
				&& anim->mRotationKeys[j].mTime < lastTime)
			{
				nextRot = &anim->mRotationKeys[j];
				lastTime = anim->mRotationKeys[j].mTime;
			}
		}

		if (!prevRot)
			prevRot = firstRot;
		if (!nextRot)
			nextRot = lastRot;

		prevVec = D3DXVECTOR3(prevPos->mValue.x, prevPos->mValue.y, prevPos->mValue.z);
		nextVec = D3DXVECTOR3(nextPos->mValue.x, nextPos->mValue.y, nextPos->mValue.z);
		prevQuat = D3DXQUATERNION(prevRot->mValue.x, prevRot->mValue.y, prevRot->mValue.z, prevRot->mValue.w);
		nextQuat = D3DXQUATERNION(nextRot->mValue.x, nextRot->mValue.y, nextRot->mValue.z, nextRot->mValue.w);

		if ((nextPos->mTime - prevPos->mTime) < 0.000001)
			lerp = prevVec;
		else
		{
			const double lerpSlp = (time - prevPos->mTime) / (nextPos->mTime - prevPos->mTime);

			if (lerpSlp < 0.000001)
				lerp = prevVec;
			else
				D3DXVec3Lerp(&lerp, &prevVec, &nextVec, (float)lerpSlp);
		}

		if ((nextRot->mTime - prevRot->mTime) < 0.000001)
			slerp = prevQuat;
		else
		{
			const double slerpSlp = (time - prevRot->mTime) / (nextRot->mTime - prevRot->mTime);

			if (slerpSlp < 0.000001)
				slerp = prevQuat;
			else
				D3DXQuaternionSlerp(&slerp, &prevQuat, &nextQuat, (float)slerpSlp);
		}

		frames[i].pos = lerp;
		frames[i].rot = slerp;
	}

	return frames;
}

void CImporter::_createBones()
{
	int boneCount = m_boneNodes.size();
	Bone* bones = new Bone[boneCount];
	memset(bones, 0, sizeof(Bone) * boneCount);

	Bone* bone = null;
	aiNode* node = null;
	int i, j, k, l;
	for (i = 0; i < boneCount; i++)
	{
		bone = &bones[i];
		node = m_boneNodes[i];

		strcpy(bone->name, node->mName.C_Str());
		bone->localTM = _getMatrix(node->mTransformation);

		if (node->mParent == m_scene->mRootNode
			|| node->mParent == null)
			bone->parentID = -1;
		else
		{
			for (j = 0; j < boneCount; j++)
			{
				if (m_boneNodes[j] == node->mParent)
				{
					bone->parentID = j;
					break;
				}
			}
		}
	}

	for (i = 0; i < boneCount; i++)
	{
		bones[i].TM = bones[i].localTM;
		if (bones[i].parentID != -1)
			bones[i].TM *= bones[bones[i].parentID].TM;
	}

	aiMesh* mesh = null;
	for (i = 0; i < m_objects.size(); i++)
	{
		for (j = 0; j < (int)m_objects[i]->mNumMeshes; j++)
		{
			mesh = m_scene->mMeshes[m_objects[i]->mMeshes[j]];
			if (mesh->HasBones())
			{
				for (k = 0; k < (int)mesh->mNumBones; k++)
				{
					bone = null;

					for (l = 0; l < boneCount; l++)
					{
						if (strcmp(bones[l].name, mesh->mBones[k]->mName.C_Str()) == 0)
						{
							bone = &bones[l];
							break;
						}
					}

					if (bone)
						bone->inverseTM = _getMatrix(mesh->mBones[k]->mOffsetMatrix);
				}
			}
		}
	}

	if (m_externBones)
	{
		CSkeleton* skel = m_mesh->m_skeleton = new CSkeleton();
		skel->m_ID = _getNewID();
		skel->m_boneCount = boneCount;
		skel->m_bones = bones;

		if (boneCount <= MAX_BONES)
			skel->m_sendVS = true;

		m_mesh->m_bones = new D3DXMATRIX[boneCount * 2];
		m_mesh->m_invBones = m_mesh->m_bones + boneCount;
		skel->ResetBones(m_mesh->m_bones, m_mesh->m_invBones);
	}
	else
	{
		m_obj3D->m_boneCount = boneCount;

		if (boneCount <= MAX_BONES)
			m_obj3D->m_sendVS = true;

		m_obj3D->m_baseBones = new D3DXMATRIX[boneCount * 2];
		m_obj3D->m_baseInvBones = m_obj3D->m_baseBones + boneCount;
		for (i = 0; i < boneCount; i++)
		{
			m_obj3D->m_baseBones[i] = bones[i].TM;
			m_obj3D->m_baseInvBones[i] = bones[i].inverseTM;
		}
	}

	m_bones = bones;
}

void CImporter::_createGMObjects()
{
	int poolSize = 0;
	aiNode* node = null;
	for (int i = 0; i < m_objects.size(); i++)
	{
		node = m_objects[i];
		const string name = string(node->mName.C_Str()).toLower();

		if (!name.endsWith("coll"))
		{
			if (name.startsWith("lod1"))
			{
				m_obj3D->m_LOD = true;
				m_obj3D->m_groups[1].objectCount++;
			}
			else if (name.startsWith("lod2"))
			{
				m_obj3D->m_LOD = true;
				m_obj3D->m_groups[2].objectCount++;
			}
			else
				m_obj3D->m_groups[0].objectCount++;
			poolSize++;
		}
	}

	if (!poolSize)
		return;

	GMObject* pool = new GMObject[poolSize];
	memset(pool, 0, sizeof(GMObject) * poolSize);
	for (int i = 0; i < (m_obj3D->m_LOD ? MAX_GROUP : 1); i++)
	{
		m_obj3D->m_groups[i].objects = pool;
		pool += m_obj3D->m_groups[i].objectCount;
	}

	int IDs[MAX_GROUP];
	memset(IDs, 0, sizeof(int) * MAX_GROUP);

	int lod = 0;

	GMObject* obj = null;
	for (int i = 0; i < m_objects.size(); i++)
	{
		node = m_objects[i];
		const string name = string(node->mName.C_Str()).toLower();

		if (name.endsWith("coll"))
		{
			obj = &m_obj3D->m_collObj;
			m_objectsIDWithLOD[node] = 0;
		}
		else
		{
			if (name.startsWith("lod1"))
				lod = 1;
			else if (name.startsWith("lod2"))
				lod = 2;
			else
				lod = 0;

			obj = &m_obj3D->m_groups[lod].objects[IDs[lod]];
			m_objectsIDWithLOD[node] = IDs[lod];
			IDs[lod]++;

			m_mapObjects[string(node->mName.C_Str())] = obj;
		}

		obj->ID = i;
		_createGMObject(obj, node);
	}
}

void CImporter::_createGMObject(GMObject* obj, aiNode* node)
{
	const string name = string(node->mName.C_Str()).toLower();

	obj->type = EGMT_NORMAL;
	for (uint i = 0; i < node->mNumMeshes; i++)
	{
		if (m_scene->mMeshes[node->mMeshes[i]]->HasBones())
		{
			obj->type = EGMT_SKIN;
			break;
		}
	}

	if (name.endsWith("light"))
		obj->light = true;
	else
		obj->light = false;

	obj->parentID = -1;
	obj->parentType = EGMT_ERROR;
	if (node->mParent != m_scene->mRootNode
		&& node->mParent != null)
	{
		if (node->mParent->mNumMeshes > 0)
		{
			obj->parentType = EGMT_NORMAL;
			obj->parentID = m_objectsIDWithLOD[node->mParent];
		}
		else
		{
			obj->parentType = EGMT_BONE;
			for (int i = 0; i < m_boneNodes.size(); i++)
			{
				if (m_boneNodes[i] == node->mParent)
				{
					obj->parentID = i;
					break;
				}
			}
		}
	}

	obj->transform = _getMatrix(node->mTransformation);

	_setMaterials(obj, node);
	_setMaterialBlocks(obj, node);

	if (obj->type == EGMT_SKIN)
	{
		_setBones(obj, node);
		_setPhysiqueVertices(obj);
	}
}

void CImporter::_setMaterials(GMObject* obj, aiNode* node)
{
	QVector<uint> materials;

	for (int i = 0; i < node->mNumMeshes; i++)
	{
		const uint id = m_scene->mMeshes[node->mMeshes[i]]->mMaterialIndex;

		if (m_scene->mMaterials[id]->GetTextureCount(aiTextureType_DIFFUSE) > 0)
		{
			if (!materials.contains(id))
				materials.push_back(id);
		}
	}

	if (materials.size() == 0)
	{
		obj->materialCount = 0;
		obj->materials = null;
		obj->material = false;
		return;
	}
	else
		obj->material = true;

	obj->materialCount = materials.size();
	obj->materials = new Material[obj->materialCount];
	memset(obj->materials, 0, sizeof(Material) * obj->materialCount);

	Material* mat = null;
	aiMaterial* aiMat = null;
	for (int i = 0; i < obj->materialCount; i++)
	{
		mat = &obj->materials[i];
		aiMat = m_scene->mMaterials[materials[i]];

		aiString path;
		aiMat->GetTexture(aiTextureType_DIFFUSE, 0, &path);
		const string texture(path.C_Str());
		const string textureFilename = QFileInfo(texture).fileName();
		const QByteArray buffer = textureFilename.toLocal8Bit();
		strcpy(mat->textureName, buffer.constData());

		aiColor3D ambiant;
		aiMat->Get(AI_MATKEY_COLOR_AMBIENT, ambiant);
		mat->material.Ambient.a = 1.0f;
		mat->material.Ambient.r = ambiant.r;
		mat->material.Ambient.g = ambiant.g;
		mat->material.Ambient.b = ambiant.b;

		aiColor3D diffuse;
		aiMat->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse);
		mat->material.Diffuse.a = 1.0f;
		mat->material.Diffuse.r = diffuse.r;
		mat->material.Diffuse.g = diffuse.g;
		mat->material.Diffuse.b = diffuse.b;

		aiColor3D specular;
		aiMat->Get(AI_MATKEY_COLOR_SPECULAR, specular);
		mat->material.Specular.a = 1.0f;
		mat->material.Specular.r = specular.r;
		mat->material.Specular.g = specular.g;
		mat->material.Specular.b = specular.b;

		aiColor3D emissive;
		aiMat->Get(AI_MATKEY_COLOR_EMISSIVE, emissive);
		mat->material.Emissive.a = 1.0f;
		mat->material.Emissive.r = emissive.r;
		mat->material.Emissive.g = emissive.g;
		mat->material.Emissive.b = emissive.b;

		float power;
		aiMat->Get(AI_MATKEY_SHININESS_STRENGTH, power);
		mat->material.Power = power;
	}
}

void CImporter::_setMaterialBlocks(GMObject* obj, aiNode* node)
{
	obj->materialBlockCount = (int)node->mNumMeshes;
	obj->materialBlocks = new MaterialBlock[obj->materialBlockCount];
	memset(obj->materialBlocks, 0, sizeof(MaterialBlock) * obj->materialBlockCount);

	obj->indexCount = 0;
	obj->faceListCount = 0;
	obj->vertexCount = 0;

	aiMesh* mesh = null;
	MaterialBlock* block = null;
	QMap<uint, int> materialIDs;
	int materialID = 0, i;
	aiMaterial* aiMat = null;

	for (i = 0; i < node->mNumMeshes; i++)
	{
		block = &obj->materialBlocks[i];
		mesh = m_scene->mMeshes[node->mMeshes[i]];

		block->primitiveCount = mesh->mNumFaces;
		block->startVertex = obj->indexCount;

		obj->indexCount += mesh->mNumFaces * 3;
		obj->vertexCount += mesh->mNumVertices;
		obj->faceListCount += mesh->mNumFaces;

		if (obj->material)
		{
			if (materialIDs.find(mesh->mMaterialIndex) == materialIDs.end())
			{
				materialIDs[mesh->mMaterialIndex] = materialID;
				materialID++;
			}

			block->materialID = materialIDs[mesh->mMaterialIndex];
			aiMat = m_scene->mMaterials[mesh->mMaterialIndex];

			float opacity;
			aiMat->Get(AI_MATKEY_OPACITY, opacity);
			block->amount = (int)(opacity * 255.0f);
			if (block->amount < 255)
				block->effect |= XE_OPACITY;

			int twoSided;
			aiMat->Get(AI_MATKEY_TWOSIDED, twoSided);
			if (twoSided)
				block->effect |= XE_2SIDE;

			if (aiMat->GetTextureCount(aiTextureType_OPACITY) > 0)
			{
				aiString textureName;
				aiMat->GetTexture(aiTextureType_OPACITY, 0, &textureName);

				if (strcmp(textureName.C_Str(), obj->materials[block->materialID].textureName) == 0)
				{
					block->effect |= XE_OPACITY;
					block->effect |= XE_2SIDE;
				}
			}
		}
		else
			block->materialID = 0;
	}

	obj->indices = new ushort[obj->indexCount + obj->vertexCount];
	obj->IIB = obj->indices + obj->indexCount;
	memset(obj->indices, 0, sizeof(ushort) * (obj->indexCount + obj->vertexCount));

	if (obj->type == EGMT_SKIN)
	{
		obj->vertices = new SkinVertex[obj->vertexCount];
		memset(obj->vertices, 0, sizeof(SkinVertex) * obj->vertexCount);
	}
	else
	{
		obj->vertices = new NormalVertex[obj->vertexCount];
		memset(obj->vertices, 0, sizeof(NormalVertex) * obj->vertexCount);
	}

	QVector<D3DXVECTOR3> vertexList;

	int indexIndex = 0,
		vertexIndex = 0,
		j, k,
		texCoordsIndex;
	for (i = 0; i < node->mNumMeshes; i++)
	{
		mesh = m_scene->mMeshes[node->mMeshes[i]];

		for (j = 0; j < mesh->mNumFaces; j++)
		{
			obj->indices[indexIndex + 0] = vertexIndex + mesh->mFaces[j].mIndices[0];
			obj->indices[indexIndex + 1] = vertexIndex + mesh->mFaces[j].mIndices[1];
			obj->indices[indexIndex + 2] = vertexIndex + mesh->mFaces[j].mIndices[2];
			indexIndex += 3;
		}

		texCoordsIndex = 0;
		for (k = 0; k < AI_MAX_NUMBER_OF_TEXTURECOORDS; k++)
		{
			if (mesh->HasTextureCoords(texCoordsIndex))
			{
				texCoordsIndex = k;
				break;
			}
		}

		if (obj->type == EGMT_SKIN)
		{
			SkinVertex* vertices = (SkinVertex*)obj->vertices;
			for (j = 0; j < mesh->mNumVertices; j++)
			{
				if (mesh->HasPositions())
					vertices[vertexIndex].p = *(D3DXVECTOR3*)&mesh->mVertices[j];
				if (mesh->HasNormals())
					vertices[vertexIndex].n = *(D3DXVECTOR3*)&mesh->mNormals[j];
				if (mesh->HasTextureCoords(texCoordsIndex))
				{
					vertices[vertexIndex].t.x = mesh->mTextureCoords[texCoordsIndex][j].x;
					vertices[vertexIndex].t.y = mesh->mTextureCoords[texCoordsIndex][j].y;
				}
				if (!vertexList.contains(vertices[vertexIndex].p))
					vertexList.push_back(vertices[vertexIndex].p);
				vertexIndex++;
			}
		}
		else
		{
			NormalVertex* vertices = (NormalVertex*)obj->vertices;
			for (j = 0; j < mesh->mNumVertices; j++)
			{
				if (mesh->HasPositions())
					vertices[vertexIndex].p = *(D3DXVECTOR3*)&mesh->mVertices[j];
				if (mesh->HasNormals())
					vertices[vertexIndex].n = *(D3DXVECTOR3*)&mesh->mNormals[j];
				if (mesh->HasTextureCoords(texCoordsIndex))
				{
					vertices[vertexIndex].t.x = mesh->mTextureCoords[texCoordsIndex][j].x;
					vertices[vertexIndex].t.y = mesh->mTextureCoords[texCoordsIndex][j].y;
				}
				if (!vertexList.contains(vertices[vertexIndex].p))
					vertexList.push_back(vertices[vertexIndex].p);
				vertexIndex++;
			}
		}
	}

	D3DXVECTOR3 bbMin(FLT_MAX, FLT_MAX, FLT_MAX);
	D3DXVECTOR3 bbMax(FLT_MIN, FLT_MIN, FLT_MIN);
	obj->vertexListCount = vertexList.size();
	obj->vertexList = new D3DXVECTOR3[obj->vertexListCount];
	D3DXVECTOR3 v;
	for (i = 0; i < obj->vertexListCount; i++)
	{
		v = vertexList[i];
		obj->vertexList[i] = v;

		if (v.x < bbMin.x)
			bbMin.x = v.x;
		if (v.y < bbMin.y)
			bbMin.y = v.y;
		if (v.z < bbMin.z)
			bbMin.z = v.z;

		if (v.x > bbMax.x)
			bbMax.x = v.x;
		if (v.y > bbMax.y)
			bbMax.y = v.y;
		if (v.z > bbMax.z)
			bbMax.z = v.z;
	}
	obj->bounds.Min = bbMin;
	obj->bounds.Max = bbMax;

	for (i = 0; i < obj->vertexCount; i++)
	{
		if (obj->type == EGMT_SKIN)
			v = ((SkinVertex*)obj->vertices)[i].p;
		else
			v = ((NormalVertex*)obj->vertices)[i].p;

		for (j = 0; j < obj->vertexListCount; j++)
		{
			if (obj->vertexList[j] == v)
			{
				obj->IIB[i] = j;
				break;
			}
		}
	}
}

void CImporter::_setBones(GMObject* obj, aiNode* node)
{
	bool sendVS = false;
	bool objUsedBones = false;
	uint i, j, k;
	aiMesh* mesh = null;

	if (m_boneNodes.size() <= MAX_BONES)
		sendVS = true;
	else
	{
		QVector<string> usedBones;
		for (i = 0; i < node->mNumMeshes; i++)
		{
			mesh = m_scene->mMeshes[node->mMeshes[i]];
			if (mesh->HasBones())
			{
				for (j = 0; j < mesh->mNumBones; j++)
				{
					const string name(mesh->mBones[j]->mName.C_Str());
					if (!usedBones.contains(name))
						usedBones.push_back(name);
				}
			}
		}
		if (usedBones.size() <= MAX_BONES)
			objUsedBones = true;
	}

	QMap<string, int> boneIDs;
	if (sendVS)
	{
		for (i = 0; i < m_boneNodes.size(); i++)
			boneIDs[m_bones[i].name] = i;
	}
	else if (objUsedBones)
	{
		QVector<string> usedBones;
		for (i = 0; i < node->mNumMeshes; i++)
		{
			mesh = m_scene->mMeshes[node->mMeshes[i]];
			if (mesh->HasBones())
			{
				for (j = 0; j < mesh->mNumBones; j++)
				{
					const string name(mesh->mBones[j]->mName.C_Str());
					if (!usedBones.contains(name))
						usedBones.push_back(name);
				}
			}
		}

		obj->usedBoneCount = usedBones.size();
		for (j = 0; j < usedBones.size(); j++)
		{
			const string name(usedBones[j]);
			int boneID = -1;

			for (k = 0; k < m_boneNodes.size(); k++)
			{
				if (m_bones[k].name == name)
				{
					boneID = k;
					break;
				}
			}

			if (boneID != -1)
			{
				obj->usedBones[j] = boneID;
				boneIDs[name] = j;
			}
		}
	}

	aiBone* bone = null;
	SkinVertex* vertices = (SkinVertex*)obj->vertices;
	for (i = 0; i < node->mNumMeshes; i++)
	{
		mesh = m_scene->mMeshes[node->mMeshes[i]];

		if (mesh->HasBones())
		{
			if (!sendVS && !objUsedBones)
			{
				boneIDs.clear();

				MaterialBlock* block = &obj->materialBlocks[i];
				block->usedBoneCount = (int)mesh->mNumBones;

				for (j = 0; j < mesh->mNumBones; j++)
				{
					bone = mesh->mBones[j];
					const string name(bone->mName.C_Str());
					int boneID = -1;

					for (k = 0; k < m_boneNodes.size(); k++)
					{
						if (m_bones[k].name == name)
						{
							boneID = k;
							break;
						}
					}

					if (boneID != -1)
					{
						block->usedBones[j] = boneID;
						boneIDs[name] = j;
					}
				}
			}

			for (j = 0; j < mesh->mNumBones; j++)
			{
				bone = mesh->mBones[j];
				const string name(bone->mName.C_Str());
				if (boneIDs.find(name) != boneIDs.end())
				{
					const ushort boneID = (ushort)boneIDs[name] * 3;
					for (k = 0; k < bone->mNumWeights; k++)
					{
						const aiVertexWeight w = bone->mWeights[k];
						if (vertices[w.mVertexId].w1 > 0.0f)
						{
							vertices[w.mVertexId].id2 = boneID;
							vertices[w.mVertexId].w2 = w.mWeight;
						}
						else
						{
							vertices[w.mVertexId].id1 = boneID;
							vertices[w.mVertexId].w1 = w.mWeight;
						}
					}
				}
			}
		}

		vertices += mesh->mNumVertices;
	}
}

void CImporter::_setPhysiqueVertices(GMObject* obj)
{
	obj->physiqueVertices = new int[obj->vertexListCount];
	memset(obj->physiqueVertices, 0, sizeof(int) * obj->vertexListCount);
	m_obj3D->m_havePhysique = true;

	int boneIDs[MAX_BONES];
	int i, j, k;

	if (obj->usedBoneCount > 0)
	{
		for (i = 0; i < obj->usedBoneCount; i++)
			boneIDs[i] = obj->usedBones[i];
	}
	else
	{
		for (i = 0; i < MAX_BONES; i++)
			boneIDs[i] = i;
	}

	SkinVertex* vertices = (SkinVertex*)obj->vertices;
	SkinVertex* vertex = null;
	int boneID;
	D3DXVECTOR3 v;
	MaterialBlock* block;
	for (i = 0; i < obj->materialBlockCount; i++)
	{
		block = &obj->materialBlocks[i];

		if (block->usedBoneCount > 0)
		{
			for (j = 0; j < block->usedBoneCount; j++)
				boneIDs[j] = block->usedBones[j];
		}

		for (j = 0; j < block->primitiveCount * 3; j++)
		{
			vertex = &vertices[obj->indices[block->startVertex + j]];

			if (vertex->w1 >= vertex->w2)
				boneID = boneIDs[vertex->id1 / 3];
			else
				boneID = boneIDs[vertex->id2 / 3];

			v = vertex->p;

			for (k = 0; k < obj->vertexListCount; k++)
			{
				if (obj->vertexList[k] == v)
				{
					obj->physiqueVertices[k] = boneID;
					break;
				}
			}
		}
	}
}

void CImporter::_scanNode(aiNode* node)
{
	aiNode* children = null;
	for (uint i = 0; i < node->mNumChildren; i++)
	{
		children = node->mChildren[i];
		if (children->mNumMeshes > 0)
			m_objects.push_back(children);
		else
			m_boneNodes.push_back(children);
		if (children->mNumChildren > 0)
			_scanNode(children);
	}
}

D3DXMATRIX CImporter::_getMatrix(const aiMatrix4x4& mat)
{
	return D3DXMATRIX(
		mat.a1, mat.b1, mat.c1, mat.d1,
		mat.a2, mat.b2, mat.c2, mat.d2,
		mat.a3, mat.b3, mat.c3, mat.d3,
		mat.a4, mat.b4, mat.c4, mat.d4
		);
}