///////////
// This file is a part of the ATools project
// Some parts of code are the property of Microsoft, Qt or Aeonsoft
// The rest is released without license and without any warranty
///////////

#include <stdafx.h>
#include "ModelViewer.h"
#include <TextureMng.h>
#include <ModelMng.h>
#include <Object3D.h>
#include <AnimatedMesh.h>
#include <ModelMng.h>
#include <MainFrame.h>
#include <Timeline.h>

struct GridVertex
{
	enum { FVF = D3DFVF_XYZ | D3DFVF_DIFFUSE };
	D3DXVECTOR3 p;
	uint c;
};

CModelViewer::CModelViewer(QWidget* parent, Qt::WindowFlags flags)
	: CD3DWidget(parent, flags)
{
	m_gridVB = null;
	m_cameraDist = 10.0f;
	m_cameraRot = D3DXVECTOR2(180.0f, 30.0f);
	m_textureMng = null;
	m_modelMng = null;
	m_mesh = null;
	m_movingCamera = false;
	m_LOD = 0;
	m_cameraPos = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
}

CModelViewer::~CModelViewer()
{
	Destroy();
}

void CModelViewer::SetFrame(int frame)
{
	if (m_mesh)
		m_mesh->SetFrame((float)frame);

	if (!IsAutoRefresh())
		RenderEnvironment();
}

void CModelViewer::SetLOD(int lod)
{
	if (lod < 0 || lod >= MAX_GROUP)
		return;

	m_LOD = lod;

	if (!IsAutoRefresh())
		RenderEnvironment();
}

bool CModelViewer::InitDeviceObjects()
{
	GridVertex vertices[17 * 4];

	float x = -8.0f;
	int vertexIndex = 0;

	for (int i = 0; i < 17; i++)
	{
		vertices[vertexIndex].p = D3DXVECTOR3(x, 0.0f, -8.0f);
		if (i == 8)
			vertices[vertexIndex].c = 0xff00ff00;
		else
			vertices[vertexIndex].c = 0xffffffff;
		vertexIndex++;
		vertices[vertexIndex].p = D3DXVECTOR3(x, 0.0f, 8.0f);
		if (i == 8)
			vertices[vertexIndex].c = 0xff00ff00;
		else
			vertices[vertexIndex].c = 0xffffffff;
		vertexIndex++;
		x += 1.0f;
	}

	float z = -8.0f;
	for (int i = 0; i < 17; i++)
	{
		vertices[vertexIndex].p = D3DXVECTOR3(-8.0f, 0.0f, z);
		if (i == 8)
			vertices[vertexIndex].c = 0xffff0000;
		else
			vertices[vertexIndex].c = 0xffffffff;
		vertexIndex++;
		vertices[vertexIndex].p = D3DXVECTOR3(8.0f, 0.0f, z);
		if (i == 8)
			vertices[vertexIndex].c = 0xffff0000;
		else
			vertices[vertexIndex].c = 0xffffffff;
		vertexIndex++;
		z += 1.0f;
	}

	if (FAILED(m_device->CreateVertexBuffer(sizeof(vertices), D3DUSAGE_WRITEONLY, GridVertex::FVF, D3DPOOL_MANAGED, &m_gridVB, NULL)))
		return false;

	void* data;
	m_gridVB->Lock(0, sizeof(vertices), &data, 0);
	memcpy(data, vertices, sizeof(vertices));
	m_gridVB->Unlock();

	m_textureMng = new CTextureMng(m_device);
	m_modelMng = new CModelMng(m_device);

	if (!CObject3D::InitStaticDeviceObjects(m_device))
		return false;

	return true;
}

void CModelViewer::DeleteDeviceObjects()
{
	CObject3D::DeleteStaticDeviceObjects();
	Release(m_gridVB);
	Delete(m_modelMng);
	Delete(m_textureMng);
}

void CModelViewer::SetMesh(CAnimatedMesh* mesh)
{
	m_mesh = mesh;
	m_cameraPos = D3DXVECTOR3(0.0f, 0.0f, 0.0f);

	if (m_mesh)
	{
		const Bounds bounds = m_mesh->GetBounds();
		m_cameraDist = bounds.Max.x + 8.0f;
		m_cameraPos.y = (bounds.Min.y + bounds.Max.y) / 2.0f;
	}
	else
		m_cameraDist = 8.0f;

	m_cameraRot = D3DXVECTOR2(180.0f, 30.0f);

	if (!IsAutoRefresh())
		RenderEnvironment();
}

bool CModelViewer::Render()
{
	m_device->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, g_global3D.backgroundColor, 1.0f, 0L);

	if (FAILED(m_device->BeginScene()))
		return false;

	D3DXMATRIX proj, view, world, invView;

	D3DXMatrixIdentity(&world);
	D3DXMatrixPerspectiveFovLH(&proj, M_PI_4, (float)width() / (float)height(), 0.1f, 400.0f);

	const float phiRadian = m_cameraRot.y * M_PI / 180.0f;
	const float thetaRadian = m_cameraRot.x * M_PI / 180.0f;
	D3DXVECTOR3 eye;
	eye.x = cos(phiRadian) * sin(thetaRadian);
	eye.y = sin(phiRadian);
	eye.z = cos(phiRadian) * cos(thetaRadian);
	eye *= m_cameraDist;
	eye += m_cameraPos;
	D3DXMatrixLookAtLH(&view, &eye, &m_cameraPos, &D3DXVECTOR3(0, 1, 0));

	m_device->SetTransform(D3DTS_VIEW, &view);
	m_device->SetTransform(D3DTS_PROJECTION, &proj);
	m_device->SetTransform(D3DTS_WORLD, &world);

	g_global3D.view = view;
	g_global3D.proj = proj;

	D3DXMatrixInverse(&invView, NULL, &view);
	invView._41 = 0.0f; invView._42 = 0.0f; invView._43 = 0.0f;
	g_global3D.invView = invView;

	g_global3D.light = false;
	g_global3D.night = false;
	g_global3D.lightVec = D3DXVECTOR4(0.0f, 0.0f, 0.0f, 0.0f);
	const D3DXVECTOR4 nullVec(1.0f, 1.0f, 1.0f, 0.0f);
	m_device->SetVertexShaderConstantF(93, (float*)&nullVec, 1);
	m_device->SetVertexShaderConstantF(94, (float*)&nullVec, 1);

	const D3DXVECTOR4 fogConst(1.0f, 1.0f, 1.0f, 100.0f);
	m_device->SetVertexShaderConstantF(95, (float*)&fogConst, 1);

	m_device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
	m_device->SetRenderState(D3DRS_ZENABLE, TRUE);
	m_device->SetRenderState(D3DRS_LIGHTING, FALSE);

	m_device->SetStreamSource(0, m_gridVB, 0, sizeof(GridVertex));
	m_device->SetFVF(GridVertex::FVF);
	m_device->SetTexture(0, NULL);

	if (g_global3D.grid)
		m_device->DrawPrimitive(D3DPT_LINELIST, 0, 17 * 2);

	m_device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	m_device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	m_device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
	m_device->SetSamplerState(1, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	m_device->SetSamplerState(1, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	m_device->SetSamplerState(1, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
	m_device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	m_device->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
	m_device->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
	m_device->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_TFACTOR);
	m_device->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
	m_device->SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	m_device->SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_CURRENT);
	m_device->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
	m_device->SetRenderState(D3DRS_ZENABLE, TRUE);
	m_device->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
	m_device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	m_device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	m_device->SetRenderState(D3DRS_ALPHAREF, 0xb0);
	m_device->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL);

	if (m_mesh)
	{
		if (IsAutoRefresh())
		{
			m_mesh->MoveFrame();
			((CMainFrame*)parent())->GetTimeline()->SetCurrentFrame((int)m_mesh->GetCurrentFrame());
		}

		m_mesh->Render(&world, m_LOD);

		if (g_global3D.renderCollisions)
		{
			m_device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_DISABLE);
			m_device->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
			m_device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
			m_device->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
			m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
			m_device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
			m_device->SetRenderState(D3DRS_LIGHTING, FALSE);
			m_device->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);

			m_mesh->RenderCollision(&world);

			m_device->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
		}
	}

	m_device->EndScene();
	return true;
}

void CModelViewer::mouseMoveEvent(QMouseEvent* event)
{
	if (m_movingCamera)
	{
		const QPoint mousePos = QPoint(event->x(), event->y());
		const QPoint mouseMove = m_lastMousePos - mousePos;

		if (QApplication::queryKeyboardModifiers() & Qt::ShiftModifier)
		{
			const float phiRadian = m_cameraRot.y * M_PI / 180.0f;
			const float thetaRadian = m_cameraRot.x * M_PI / 180.0f;
			D3DXVECTOR3 eye, moveX, moveY;
			eye.x = cos(phiRadian) * sin(thetaRadian);
			eye.y = sin(phiRadian);
			eye.z = cos(phiRadian) * cos(thetaRadian);
			D3DXVec3Cross(&moveY, &eye, &D3DXVECTOR3(0.0f, 1.0f, 0.0f));
			D3DXVec3Normalize(&moveY, &moveY);
			D3DXVec3Cross(&moveX, &eye, &moveY);
			D3DXVec3Normalize(&moveX, &moveX);
			m_cameraPos += moveX * ((float)mouseMove.y() / 750.0f * (m_cameraDist + 1.0f));
			m_cameraPos += moveY * ((float)mouseMove.x() / 750.0f * (m_cameraDist + 1.0f));
		}
		else
		{
			m_cameraRot.x -= (float)mouseMove.x() / 4.0f;
			m_cameraRot.y -= (float)mouseMove.y() / 4.0f;

			if (m_cameraRot.y > 89.9f)
				m_cameraRot.y = 89.9f;
			else if (m_cameraRot.y < -89.9f)
				m_cameraRot.y = -89.9f;
		}

		m_lastMousePos = mousePos;

		if (!IsAutoRefresh())
			RenderEnvironment();

		event->accept();
	}
}

void CModelViewer::wheelEvent(QWheelEvent* event)
{
	if (event->orientation() == Qt::Vertical)
	{
		m_cameraDist -= (float)event->delta() / 120.0f;

		if (m_cameraDist < 0.1f)
			m_cameraDist = 0.1f;
		else if (m_cameraDist > 400.0f)
			m_cameraDist = 400.0f;

		if (!IsAutoRefresh())
			RenderEnvironment();

		event->accept();
	}
}

void CModelViewer::mousePressEvent(QMouseEvent* event)
{
	if (event->button() == Qt::RightButton || event->button() == Qt::MiddleButton)
	{
		m_movingCamera = true;
		m_lastMousePos = QPoint(event->x(), event->y());

		event->accept();
	}
}

void CModelViewer::mouseReleaseEvent(QMouseEvent* event)
{
	if (event->button() == Qt::RightButton || event->button() == Qt::MiddleButton)
	{
		m_movingCamera = false;
		event->accept();
	}
}