///////////
// This file is a part of the ATools project
// Some parts of code are the property of Microsoft, Qt or Aeonsoft
// The rest is released without license and without any warranty
///////////

#ifndef MODELVIEWER_H
#define MODELVIEWER_H

#include <D3DWidget.h>

class CTextureMng;
class CModelMng;
class CAnimatedMesh;

class CModelViewer : public CD3DWidget
{
	Q_OBJECT

public:
	CModelViewer(QWidget* parent = null, Qt::WindowFlags flags = 0);
	~CModelViewer();

	virtual bool InitDeviceObjects();
	virtual void DeleteDeviceObjects();
	virtual bool Render();

	virtual void mouseMoveEvent(QMouseEvent * event);
	virtual void wheelEvent(QWheelEvent *event);
	virtual void mousePressEvent(QMouseEvent * event);
	virtual void mouseReleaseEvent(QMouseEvent * event);

	void SetMesh(CAnimatedMesh* mesh);
	void SetLOD(int lod);

public slots:
	void SetFrame(int frame);

private:
	LPDIRECT3DVERTEXBUFFER9 m_gridVB;
	float m_cameraDist;
	D3DXVECTOR2 m_cameraRot;
	QPoint m_lastMousePos;
	bool m_movingCamera;
	int m_LOD;
	D3DXVECTOR3 m_cameraPos;

	CTextureMng* m_textureMng;
	CModelMng* m_modelMng;
	CAnimatedMesh* m_mesh;
};

#endif // MODELVIEWER_H