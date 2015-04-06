///////////
// This file is a part of the ATools project
// Some parts of code are the property of Microsoft, Qt or Aeonsoft
// The rest is released without license and without any warranty
///////////

#ifndef IMPORTER_H
#define IMPORTER_H

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

class CAnimatedMesh;
class CObject3D;
struct GMObject;
struct Bone;
struct TMAnimation;

class CImporter
{
public:
	CImporter(CAnimatedMesh* mesh);
	~CImporter();

	bool Import(const string& filename);

private:
	aiScene* m_scene;

	CAnimatedMesh* m_mesh;
	CObject3D* m_obj3D;
	bool m_externBones;
	Bone* m_bones;

	QVector<aiNode*> m_objects;
	QMap<aiNode*, int> m_objectsIDWithLOD;
	QVector<aiNode*> m_boneNodes;
	QMap<string, GMObject*> m_mapObjects;

	int m_frameCount;
	double m_ticksPerSecond;

private:
	void _importScene();
	void _scanNode(aiNode* node);
	void _createBones();
	void _createGMObjects();
	void _createGMObject(GMObject* obj, aiNode* node);
	void _setMaterials(GMObject* obj, aiNode* node);
	void _setMaterialBlocks(GMObject* obj, aiNode* node);
	void _setBones(GMObject* obj, aiNode* node);
	void _createAnimations();
	TMAnimation* _createAnimation(aiNodeAnim* anim);
	void _setPhysiqueVertices(GMObject* obj);
	void _calculateBounds();
	void _preTransformVertices(GMObject* obj);

	D3DXMATRIX _getMatrix(const aiMatrix4x4& mat);

	int _getNewID()
	{
		int id = rand();
		if (id < 0)
			return -id;
		else if (id == 0)
			return 1;
		else
			return id;
	}
};

#endif // IMPORTER_H