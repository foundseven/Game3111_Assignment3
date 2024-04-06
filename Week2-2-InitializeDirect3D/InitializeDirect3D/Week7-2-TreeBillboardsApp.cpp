/** @file Week7-2-TreeBillboardsApp.cpp
 * 
 *   Controls:
 *   WASD To Move,  L Mouse Button to look around.
 *
 *  @author Hooman Salamat
 */

#include "../../Common/d3dApp.h"
#include "../../Common/MathHelper.h"
#include "../../Common/UploadBuffer.h"
#include "../../Common/GeometryGenerator.h"
#include "../../Common/Camera.h"
#include "FrameResource.h"
#include "Waves.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")

const int gNumFrameResources = 3;

// Lightweight structure stores parameters to draw a shape.  This will
// vary from app-to-app.
struct RenderItem
{
	RenderItem() = default;

    // World matrix of the shape that describes the object's local space
    // relative to the world space, which defines the position, orientation,
    // and scale of the object in the world.
    XMFLOAT4X4 World = MathHelper::Identity4x4();

	XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();

	// Dirty flag indicating the object data has changed and we need to update the constant buffer.
	// Because we have an object cbuffer for each FrameResource, we have to apply the
	// update to each FrameResource.  Thus, when we modify obect data we should set 
	// NumFramesDirty = gNumFrameResources so that each frame resource gets the update.
	int NumFramesDirty = gNumFrameResources;

	// Index into GPU constant buffer corresponding to the ObjectCB for this render item.
	UINT ObjCBIndex = -1;

	Material* Mat = nullptr;
	MeshGeometry* Geo = nullptr;

    // Primitive topology.
    D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    // DrawIndexedInstanced parameters.
    UINT IndexCount = 0;
    UINT StartIndexLocation = 0;
    int BaseVertexLocation = 0;
};

enum class RenderLayer : int
{
	Opaque = 0,
	Transparent,
	AlphaTested,
	AlphaTestedTreeSprites,
	Count
};

class TreeBillboardsApp : public D3DApp
{
public:
    TreeBillboardsApp(HINSTANCE hInstance);
    TreeBillboardsApp(const TreeBillboardsApp& rhs) = delete;
    TreeBillboardsApp& operator=(const TreeBillboardsApp& rhs) = delete;
    ~TreeBillboardsApp();

    virtual bool Initialize()override;

private:
    virtual void OnResize()override;
    virtual void Update(const GameTimer& gt)override;
    virtual void Draw(const GameTimer& gt)override;

    virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
    virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
    virtual void OnMouseMove(WPARAM btnState, int x, int y)override;

    void OnKeyboardInput(const GameTimer& gt);
	void UpdateCamera(const GameTimer& gt);
	void AnimateMaterials(const GameTimer& gt);
	void UpdateObjectCBs(const GameTimer& gt);
	void UpdateMaterialCBs(const GameTimer& gt);
	void UpdateMainPassCB(const GameTimer& gt);
	void UpdateWaves(const GameTimer& gt); 

	void LoadTextures();
    void BuildRootSignature();
	void BuildDescriptorHeaps();
    void BuildShadersAndInputLayouts();
    void BuildLandGeometry();
    void BuildWavesGeometry();
	void BuildBoxGeometry();
	void BuildTreeSpritesGeometry();
    void BuildPSOs();
    void BuildFrameResources();
    void BuildMaterials();
    void BuildRenderItems();
	//BUILDING COLLISION
	//	bool CheckCameraCollision(FXMVECTOR predictPos);
    void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems);
	void CreateNewObject(const char* item, XMMATRIX p, XMMATRIX q, XMMATRIX r, UINT ObjIndex, const char* material);
	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers();

    float GetHillsHeight(float x, float z)const;
    XMFLOAT3 GetHillsNormal(float x, float z)const;

private:

    std::vector<std::unique_ptr<FrameResource>> mFrameResources;
    FrameResource* mCurrFrameResource = nullptr;
    int mCurrFrameResourceIndex = 0;

    UINT mCbvSrvDescriptorSize = 0;

    ComPtr<ID3D12RootSignature> mRootSignature = nullptr;

	ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap = nullptr;

	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mGeometries;
	std::unordered_map<std::string, std::unique_ptr<Material>> mMaterials;
	std::unordered_map<std::string, std::unique_ptr<Texture>> mTextures;
	std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders;
	std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> mPSOs;

    std::vector<D3D12_INPUT_ELEMENT_DESC> mStdInputLayout;
	std::vector<D3D12_INPUT_ELEMENT_DESC> mTreeSpriteInputLayout;

    RenderItem* mWavesRitem = nullptr;

	// List of all the render items.
	std::vector<std::unique_ptr<RenderItem>> mAllRitems;

	// Render items divided by PSO.
	std::vector<RenderItem*> mRitemLayer[(int)RenderLayer::Count];

	std::unique_ptr<Waves> mWaves;

    PassConstants mMainPassCB;

	//Adding in First Person Camera
	//Camera mCamera;
	Camera mCamera;
	float mCameraSpeed = 10.f;

	//Bounding boxes for collision



	//I WONT NEED THESE
	//XMFLOAT3 mEyePos = { 0.0f, 0.0f, 0.0f };
	//XMFLOAT4X4 mView = MathHelper::Identity4x4();
	//XMFLOAT4X4 mProj = MathHelper::Identity4x4();

    //float mTheta = 1.5f*XM_PI;
    //float mPhi = XM_PIDIV2 - 0.1f;
    //float mRadius = 50.0f;

    POINT mLastMousePos;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
    PSTR cmdLine, int showCmd)
{
    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    try
    {
        TreeBillboardsApp theApp(hInstance);
        if(!theApp.Initialize())
            return 0;

        return theApp.Run();
    }
    catch(DxException& e)
    {
        MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
        return 0;
    }
}
///////////////////////// INIT THE TREES ////////////////////////////////////

TreeBillboardsApp::TreeBillboardsApp(HINSTANCE hInstance)
    : D3DApp(hInstance)
{
}

TreeBillboardsApp::~TreeBillboardsApp()
{
    if(md3dDevice != nullptr)
        FlushCommandQueue();
}

//CREATEITEM?
void TreeBillboardsApp::CreateNewObject(const char* item, XMMATRIX p, XMMATRIX q, XMMATRIX r, UINT ObjIndex, const char* material)
{
	auto RightWall = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&RightWall->World, p * q * r);
	RightWall->ObjCBIndex = ObjIndex;
	RightWall->Mat = mMaterials[material].get();//
	RightWall->Geo = mGeometries["boxGeo"].get();

	RightWall->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	RightWall->IndexCount = RightWall->Geo->DrawArgs[item].IndexCount;
	RightWall->StartIndexLocation = RightWall->Geo->DrawArgs[item].StartIndexLocation;
	RightWall->BaseVertexLocation = RightWall->Geo->DrawArgs[item].BaseVertexLocation;
	//mAllRitems.push_back(std::move(RightWall));
	mRitemLayer[(int)RenderLayer::Opaque].push_back(RightWall.get());
	mAllRitems.push_back(std::move(RightWall));
}


///////////////////////// INIT ////////////////////////////////////

bool TreeBillboardsApp::Initialize()
{
    if(!D3DApp::Initialize())
        return false;

    // Reset the command list to prep for initialization commands.
    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

    // Get the increment size of a descriptor in this heap type.  This is hardware specific, 
	// so we have to query this information.
    mCbvSrvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	//setting the camera POS
	mCamera.SetPosition(0.0f, 2.0f, 0.0f);

	//bounding box POS 


    mWaves = std::make_unique<Waves>(128, 128, 1.0f, 0.03f, 4.0f, 0.2f); //change water size
 
	LoadTextures();
    BuildRootSignature();
	BuildDescriptorHeaps();
    BuildShadersAndInputLayouts();
    BuildLandGeometry();
    BuildWavesGeometry();
	BuildBoxGeometry();
	BuildTreeSpritesGeometry();
	BuildMaterials();
    BuildRenderItems();
    BuildFrameResources();
    BuildPSOs();

    // Execute the initialization commands.
    ThrowIfFailed(mCommandList->Close());
    ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

    // Wait until initialization is complete.
    FlushCommandQueue();

    return true;
}
 
void TreeBillboardsApp::OnResize()
{
    D3DApp::OnResize();

    // The window resized, so update the aspect ratio and recompute the projection matrix.
    //XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f*MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
    //XMStoreFloat4x4(&mProj, P);

	//camera resize
	//When the window is resized, we no longer rebuild the projection matrix explicitly, 
	//and instead delegate the work to the Camera class with SetLens:
	mCamera.SetLens(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);

}
///////////////////////// UPDATE ////////////////////////////////////

void TreeBillboardsApp::Update(const GameTimer& gt)
{
    OnKeyboardInput(gt);
	//UpdateCamera(gt);

    // Cycle through the circular frame resource array.
    mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % gNumFrameResources;
    mCurrFrameResource = mFrameResources[mCurrFrameResourceIndex].get();

    // Has the GPU finished processing the commands of the current frame resource?
    // If not, wait until the GPU has completed commands up to this fence point.
    if(mCurrFrameResource->Fence != 0 && mFence->GetCompletedValue() < mCurrFrameResource->Fence)
    {
        HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);
        ThrowIfFailed(mFence->SetEventOnCompletion(mCurrFrameResource->Fence, eventHandle));
        WaitForSingleObject(eventHandle, INFINITE);
        CloseHandle(eventHandle);
    }

	AnimateMaterials(gt);
	UpdateObjectCBs(gt);
	UpdateMaterialCBs(gt);
	UpdateMainPassCB(gt);
    UpdateWaves(gt);
}
///////////////////////// DRAW ////////////////////////////////////

void TreeBillboardsApp::Draw(const GameTimer& gt)
{
    auto cmdListAlloc = mCurrFrameResource->CmdListAlloc;

    // Reuse the memory associated with command recording.
    // We can only reset when the associated command lists have finished execution on the GPU.
    ThrowIfFailed(cmdListAlloc->Reset());

    // A command list can be reset after it has been added to the command queue via ExecuteCommandList.
    // Reusing the command list reuses memory.
    ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), mPSOs["opaque"].Get()));

    mCommandList->RSSetViewports(1, &mScreenViewport);
    mCommandList->RSSetScissorRects(1, &mScissorRect);

    // Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

    // Clear the back buffer and depth buffer.
    mCommandList->ClearRenderTargetView(CurrentBackBufferView(), (float*)&mMainPassCB.FogColor, 0, nullptr);
    mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

    // Specify the buffers we are going to render to.
    mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

	ID3D12DescriptorHeap* descriptorHeaps[] = { mSrvDescriptorHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

	auto passCB = mCurrFrameResource->PassCB->Resource();
	mCommandList->SetGraphicsRootConstantBufferView(2, passCB->GetGPUVirtualAddress());

    DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Opaque]);

	mCommandList->SetPipelineState(mPSOs["alphaTested"].Get());
	DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::AlphaTested]);

	mCommandList->SetPipelineState(mPSOs["treeSprites"].Get());
	DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::AlphaTestedTreeSprites]);

	mCommandList->SetPipelineState(mPSOs["transparent"].Get());
	DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Transparent]);

    // Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

    // Done recording commands.
    ThrowIfFailed(mCommandList->Close());

    // Add the command list to the queue for execution.
    ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

    // Swap the back and front buffers
    ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

    // Advance the fence value to mark commands up to this fence point.
    mCurrFrameResource->Fence = ++mCurrentFence;

    // Add an instruction to the command queue to set a new fence point. 
    // Because we are on the GPU timeline, the new fence point won't be 
    // set until the GPU finishes processing all the commands prior to this Signal().
    mCommandQueue->Signal(mFence.Get(), mCurrentFence);
}
///////////////////////// MOVING DOWN WITH THE MOUSE ////////////////////////////////////

void TreeBillboardsApp::OnMouseDown(WPARAM btnState, int x, int y)
{
    mLastMousePos.x = x;
    mLastMousePos.y = y;

    SetCapture(mhMainWnd);
}
///////////////////////// MOVING UP WITH THE MOUSE ////////////////////////////////////

void TreeBillboardsApp::OnMouseUp(WPARAM btnState, int x, int y)
{
    ReleaseCapture();
}
///////////////////////// TELLING THE MOUSE TO MOVE ACCORDINGLY ////////////////////////////////////

void TreeBillboardsApp::OnMouseMove(WPARAM btnState, int x, int y)
{
    if((btnState & MK_LBUTTON) != 0)
    {
        // Make each pixel correspond to a quarter of a degree.
        float dx = XMConvertToRadians(0.25f*static_cast<float>(x - mLastMousePos.x));
        float dy = XMConvertToRadians(0.25f*static_cast<float>(y - mLastMousePos.y));

        // Update angles based on input to orbit camera around box.
        //mTheta += dx;
        //mPhi += dy;

        // Restrict the angle mPhi.
        //mPhi = MathHelper::Clamp(mPhi, 0.1f, MathHelper::Pi - 0.1f);
		
		//step4: Instead of updating the angles based on input to orbit camera around scene, 
		//we rotate the cameras look direction:
		mCamera.Pitch(dy);
		mCamera.RotateY(dx);
    }
    //else if((btnState & MK_RBUTTON) != 0)
    //{
    //    // Make each pixel correspond to 0.2 unit in the scene.
    //    float dx = 0.2f*static_cast<float>(x - mLastMousePos.x);
    //    float dy = 0.2f*static_cast<float>(y - mLastMousePos.y);

    //    // Update the camera radius based on input.
    //    mRadius += dx - dy;

    //    // Restrict the radius.
    //    mRadius = MathHelper::Clamp(mRadius, 5.0f, 150.0f);
    //}

    mLastMousePos.x = x;
    mLastMousePos.y = y;
}
 
void TreeBillboardsApp::OnKeyboardInput(const GameTimer& gt)
{
	//implement a func that will allow us to 
	const float dt = gt.DeltaTime();

	if (GetAsyncKeyState('W') & 0x8000) //most significant bit (MSB) is 1 when key is pressed (1000 000 000 000)
		mCamera.Walk(10.0f * dt);

	if (GetAsyncKeyState('S') & 0x8000)
		mCamera.Walk(-10.0f * dt);

	if (GetAsyncKeyState('A') & 0x8000)
		mCamera.Strafe(-10.0f * dt);

	if (GetAsyncKeyState('D') & 0x8000)
		mCamera.Strafe(10.0f * dt);

	//lets add q and e to go up and down

	mCamera.UpdateViewMatrix();

}
///////////////////////// UPDATING CAMERA ////////////////////////////////////

//void TreeBillboardsApp::UpdateCamera(const GameTimer& gt)
//{
//	// Convert Spherical to Cartesian coordinates.
//	mEyePos.x = mRadius*sinf(mPhi)*cosf(mTheta);
//	mEyePos.z = mRadius*sinf(mPhi)*sinf(mTheta);
//	mEyePos.y = mRadius*cosf(mPhi);
//
//	// Build the view matrix.
//	XMVECTOR pos = XMVectorSet(mEyePos.x, mEyePos.y, mEyePos.z, 1.0f);
//	XMVECTOR target = XMVectorZero();
//	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
//
//	XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
//	XMStoreFloat4x4(&mView, view);
//}
///////////////////////// SETTING UP ANIMATIONS ////////////////////////////////////

void TreeBillboardsApp::AnimateMaterials(const GameTimer& gt)
{
	// Scroll the water material texture coordinates.
	auto waterMat = mMaterials["water"].get();

	float& tu = waterMat->MatTransform(3, 0);
	float& tv = waterMat->MatTransform(3, 1);

	tu += 0.1f * gt.DeltaTime();
	tv += 0.02f * gt.DeltaTime();

	if(tu >= 1.0f)
		tu -= 1.0f;

	if(tv >= 1.0f)
		tv -= 1.0f;

	waterMat->MatTransform(3, 0) = tu;
	waterMat->MatTransform(3, 1) = tv;

	// Material has changed, so need to update cbuffer.
	waterMat->NumFramesDirty = gNumFrameResources;
}

void TreeBillboardsApp::UpdateObjectCBs(const GameTimer& gt)
{
	auto currObjectCB = mCurrFrameResource->ObjectCB.get();
	for(auto& e : mAllRitems)
	{
		// Only update the cbuffer data if the constants have changed.  
		// This needs to be tracked per frame resource.
		if(e->NumFramesDirty > 0)
		{
			XMMATRIX world = XMLoadFloat4x4(&e->World);
			XMMATRIX texTransform = XMLoadFloat4x4(&e->TexTransform);

			ObjectConstants objConstants;
			XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));
			XMStoreFloat4x4(&objConstants.TexTransform, XMMatrixTranspose(texTransform));

			currObjectCB->CopyData(e->ObjCBIndex, objConstants);

			// Next FrameResource need to be updated too.
			e->NumFramesDirty--;
		}
	}
}

void TreeBillboardsApp::UpdateMaterialCBs(const GameTimer& gt)
{
	auto currMaterialCB = mCurrFrameResource->MaterialCB.get();
	for(auto& e : mMaterials)
	{
		// Only update the cbuffer data if the constants have changed.  If the cbuffer
		// data changes, it needs to be updated for each FrameResource.
		Material* mat = e.second.get();
		if(mat->NumFramesDirty > 0)
		{
			XMMATRIX matTransform = XMLoadFloat4x4(&mat->MatTransform);

			MaterialConstants matConstants;
			matConstants.DiffuseAlbedo = mat->DiffuseAlbedo;
			matConstants.FresnelR0 = mat->FresnelR0;
			matConstants.Roughness = mat->Roughness;
			XMStoreFloat4x4(&matConstants.MatTransform, XMMatrixTranspose(matTransform));

			currMaterialCB->CopyData(mat->MatCBIndex, matConstants);

			// Next FrameResource need to be updated too.
			mat->NumFramesDirty--;
		}
	}
}
///////////////////////// UPDATING MAIN PASS ////////////////////////////////////

void TreeBillboardsApp::UpdateMainPassCB(const GameTimer& gt)
{
	//XMMATRIX view = XMLoadFloat4x4(&mView);
	//XMMATRIX proj = XMLoadFloat4x4(&mProj);
	// 
	//First Person Camera
	XMMATRIX view = mCamera.GetView();
	XMMATRIX proj = mCamera.GetProj();

	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
	XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
	XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

	XMStoreFloat4x4(&mMainPassCB.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&mMainPassCB.InvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&mMainPassCB.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&mMainPassCB.InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&mMainPassCB.ViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&mMainPassCB.InvViewProj, XMMatrixTranspose(invViewProj));
	//mMainPassCB.EyePosW = mEyePos;
	mMainPassCB.EyePosW = mCamera.GetPosition3f();

	mMainPassCB.RenderTargetSize = XMFLOAT2((float)mClientWidth, (float)mClientHeight);
	mMainPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / mClientWidth, 1.0f / mClientHeight);
	mMainPassCB.NearZ = 1.0f;
	mMainPassCB.FarZ = 1000.0f;
	mMainPassCB.TotalTime = gt.TotalTime();
	mMainPassCB.DeltaTime = gt.DeltaTime();
	mMainPassCB.AmbientLight = { 0.47f, 0.47f, 0.47f, 1.2f };
	mMainPassCB.Lights[0].Direction = { 0.57735f, -0.57735f, 0.57735f };
	mMainPassCB.Lights[0].Strength = { 1.2f, 0.4f, 0.4f };
	mMainPassCB.Lights[1].Direction = { -0.57735f, -0.57735f, 0.57735f };
	mMainPassCB.Lights[1].Strength = { 0.02f, 0.02f, 0.02f };
	mMainPassCB.Lights[2].Direction = { 0.0f, -0.707f, -0.707f };
	mMainPassCB.Lights[2].Strength = { 0.05f, 0.05f, 0.05f };

	//ADDING SOME MORE LIGHTS HERE 
	//spot light
	mMainPassCB.Lights[3].Position = { 0.0f, 10.0f, 0.0f };
	mMainPassCB.Lights[3].Direction = { 0.0f, 0.0f, 0.0f };
	mMainPassCB.Lights[2].Strength = { 1.1f, 0.0f, 0.2f };
	mMainPassCB.Lights[2].SpotPower = 1.7f;

	//one more
	mMainPassCB.Lights[4].Position = { 0.0f, 10.0f, 0.0f };
	mMainPassCB.Lights[4].Strength = { 1000.1f, 0.0f, 100.2f };

	auto currPassCB = mCurrFrameResource->PassCB.get();
	currPassCB->CopyData(0, mMainPassCB);
}
///////////////////////// UPDATING WAVES ////////////////////////////////////
void TreeBillboardsApp::UpdateWaves(const GameTimer& gt)
{
	// Every quarter second, generate a random wave.
	static float t_base = 0.0f;
	if((mTimer.TotalTime() - t_base) >= 0.25f)
	{
		t_base += 0.25f;

		int i = MathHelper::Rand(4, mWaves->RowCount() - 5);
		int j = MathHelper::Rand(4, mWaves->ColumnCount() - 5);

		float r = MathHelper::RandF(0.2f, 0.5f);

		mWaves->Disturb(i, j, r);
	}

	// Update the wave simulation.
	mWaves->Update(gt.DeltaTime());

	// Update the wave vertex buffer with the new solution.
	auto currWavesVB = mCurrFrameResource->WavesVB.get();
	for(int i = 0; i < mWaves->VertexCount(); ++i)
	{
		Vertex v;

		v.Pos = mWaves->Position(i);
		v.Normal = mWaves->Normal(i);
		
		// Derive tex-coords from position by 
		// mapping [-w/2,w/2] --> [0,1]
		v.TexC.x = 0.5f + v.Pos.x / mWaves->Width();
		v.TexC.y = 0.5f - v.Pos.z / mWaves->Depth();

		currWavesVB->CopyData(i, v);
	}

	// Set the dynamic VB of the wave renderitem to the current frame VB.
	mWavesRitem->Geo->VertexBufferGPU = currWavesVB->Resource();
}
///////////////////////// LOADING TEXTURES ////////////////////////////////////

void TreeBillboardsApp::LoadTextures()
{
	//create new textures in here
	auto grassTex = std::make_unique<Texture>();
	grassTex->Name = "grassTex";
	grassTex->Filename = L"../../Textures/grass.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), grassTex->Filename.c_str(),
		grassTex->Resource, grassTex->UploadHeap));

	auto waterTex = std::make_unique<Texture>();
	waterTex->Name = "waterTex";
	waterTex->Filename = L"../../Textures/water1.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), waterTex->Filename.c_str(),
		waterTex->Resource, waterTex->UploadHeap));

	auto fenceTex = std::make_unique<Texture>();
	fenceTex->Name = "fenceTex";
	fenceTex->Filename = L"../../Textures/WireFence.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), fenceTex->Filename.c_str(),
		fenceTex->Resource, fenceTex->UploadHeap));

	//ADDING MORE TEXTURES
	// stone texture
	auto stoneTex = std::make_unique<Texture>();
	stoneTex->Name = "stoneTex";
	stoneTex->Filename = L"../../Textures/stone.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), stoneTex->Filename.c_str(),
		stoneTex->Resource, stoneTex->UploadHeap));
	
	//marble mat
	auto marbleTex = std::make_unique<Texture>();
	marbleTex->Name = "marbleTex";
	marbleTex->Filename = L"../../Textures/marble.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), marbleTex->Filename.c_str(),
		marbleTex->Resource, marbleTex->UploadHeap));

	//circle mat - sun
	auto sunTex = std::make_unique<Texture>();
	sunTex->Name = "sunTex";
	sunTex->Filename = L"../../Textures/sun.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), sunTex->Filename.c_str(),
		sunTex->Resource, sunTex->UploadHeap));

	//diamond mat
	auto diamondTex = std::make_unique<Texture>();
	diamondTex->Name = "diamondTex";
	diamondTex->Filename = L"../../Textures/diamonds.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), diamondTex->Filename.c_str(),
		diamondTex->Resource, diamondTex->UploadHeap));

	//leaves mat
	auto bushTex = std::make_unique<Texture>();
	bushTex->Name = "bushTex";
	bushTex->Filename = L"../../Textures/bush.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), bushTex->Filename.c_str(),
		bushTex->Resource, bushTex->UploadHeap));

	//wood mat
	auto woodTex = std::make_unique<Texture>();
	woodTex->Name = "woodTex";
	woodTex->Filename = L"../../Textures/wood.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), woodTex->Filename.c_str(),
		woodTex->Resource, woodTex->UploadHeap));

	//trees mat
	auto treeArrayTex = std::make_unique<Texture>();
	treeArrayTex->Name = "treeArrayTex";
	treeArrayTex->Filename = L"../../Textures/treeArray.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), treeArrayTex->Filename.c_str(),
		treeArrayTex->Resource, treeArrayTex->UploadHeap));


	mTextures[grassTex->Name] = std::move(grassTex);
	mTextures[waterTex->Name] = std::move(waterTex);
	mTextures[fenceTex->Name] = std::move(fenceTex);
	//stone 
	mTextures[stoneTex->Name] = std::move(stoneTex);
	//marble
	mTextures[marbleTex->Name] = std::move(marbleTex);
	//sun
	mTextures[sunTex->Name] = std::move(sunTex);
	//diamond
	mTextures[diamondTex->Name] = std::move(diamondTex);
	//bush
	mTextures[bushTex->Name] = std::move(bushTex);
	//wood
	mTextures[woodTex->Name] = std::move(woodTex);
	//tree
	mTextures[treeArrayTex->Name] = std::move(treeArrayTex);
}

void TreeBillboardsApp::BuildRootSignature()
{
	CD3DX12_DESCRIPTOR_RANGE texTable;
	texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

    // Root parameter can be a table, root descriptor or root constants.
    CD3DX12_ROOT_PARAMETER slotRootParameter[4];

	// Perfomance TIP: Order from most frequent to least frequent.
	slotRootParameter[0].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL);
    slotRootParameter[1].InitAsConstantBufferView(0);
    slotRootParameter[2].InitAsConstantBufferView(1);
    slotRootParameter[3].InitAsConstantBufferView(2);

	auto staticSamplers = GetStaticSamplers();

    // A root signature is an array of root parameters.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(4, slotRootParameter,
		(UINT)staticSamplers.size(), staticSamplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    // create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
    ComPtr<ID3DBlob> serializedRootSig = nullptr;
    ComPtr<ID3DBlob> errorBlob = nullptr;
    HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
        serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

    if(errorBlob != nullptr)
    {
        ::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
    }
    ThrowIfFailed(hr);

    ThrowIfFailed(md3dDevice->CreateRootSignature(
		0,
        serializedRootSig->GetBufferPointer(),
        serializedRootSig->GetBufferSize(),
        IID_PPV_ARGS(mRootSignature.GetAddressOf())));
}

void TreeBillboardsApp::BuildDescriptorHeaps()
{
	//
	// Create the SRV heap.
	//
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = 10;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mSrvDescriptorHeap)));

	//
	// Fill out the heap with actual descriptors.
	//
	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	auto grassTex = mTextures["grassTex"]->Resource;
	auto waterTex = mTextures["waterTex"]->Resource;
	auto fenceTex = mTextures["fenceTex"]->Resource;
	//adding new heaps here
	//STONE HEAP//
	auto stoneTex = mTextures["stoneTex"]->Resource;
	//MARBLE HEAP//
	auto marbleTex = mTextures["marbleTex"]->Resource;
	//SUN HEAP//
	auto sunTex = mTextures["sunTex"]->Resource;
	//DIAMOND HEAP//
	auto diamondTex = mTextures["diamondTex"]->Resource;
	//BUSH HEAP//
	auto bushTex = mTextures["bushTex"]->Resource;
	//WOOD HEAP//
	auto woodTex = mTextures["woodTex"]->Resource;
	// TREE HEAP //
	auto treeArrayTex = mTextures["treeArrayTex"]->Resource;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	srvDesc.Format = grassTex->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = -1;
	md3dDevice->CreateShaderResourceView(grassTex.Get(), &srvDesc, hDescriptor);

	// next descriptor
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	srvDesc.Format = waterTex->GetDesc().Format;
	md3dDevice->CreateShaderResourceView(waterTex.Get(), &srvDesc, hDescriptor);

	// next descriptor
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	srvDesc.Format = fenceTex->GetDesc().Format;
	md3dDevice->CreateShaderResourceView(fenceTex.Get(), &srvDesc, hDescriptor);

	//next descriptor (stone)
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	srvDesc.Format = stoneTex->GetDesc().Format;
	md3dDevice->CreateShaderResourceView(stoneTex.Get(), &srvDesc, hDescriptor);

	//next descriptor (marble)
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	srvDesc.Format = marbleTex->GetDesc().Format;
	srvDesc.Texture2D.MipLevels = marbleTex->GetDesc().MipLevels;
	md3dDevice->CreateShaderResourceView(marbleTex.Get(), &srvDesc, hDescriptor);

	//next descriptor (sun)
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	srvDesc.Format = sunTex->GetDesc().Format;
	srvDesc.Texture2D.MipLevels = sunTex->GetDesc().MipLevels;
	md3dDevice->CreateShaderResourceView(sunTex.Get(), &srvDesc, hDescriptor);

	//next descriptor (diamond)
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	srvDesc.Format = diamondTex->GetDesc().Format;
	srvDesc.Texture2D.MipLevels = diamondTex->GetDesc().MipLevels;
	md3dDevice->CreateShaderResourceView(diamondTex.Get(), &srvDesc, hDescriptor);

	//next descriptor (bush)
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	srvDesc.Format = bushTex->GetDesc().Format;
	srvDesc.Texture2D.MipLevels = bushTex->GetDesc().MipLevels;
	md3dDevice->CreateShaderResourceView(bushTex.Get(), &srvDesc, hDescriptor);

	//next descriptor (wood)
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	srvDesc.Format = woodTex->GetDesc().Format;
	srvDesc.Texture2D.MipLevels = woodTex->GetDesc().MipLevels;
	md3dDevice->CreateShaderResourceView(woodTex.Get(), &srvDesc, hDescriptor);

	// next descriptor
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	auto desc = treeArrayTex->GetDesc();
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
	srvDesc.Format = treeArrayTex->GetDesc().Format;
	srvDesc.Texture2DArray.MostDetailedMip = 0;
	srvDesc.Texture2DArray.MipLevels = -1;
	srvDesc.Texture2DArray.FirstArraySlice = 0;
	srvDesc.Texture2DArray.ArraySize = treeArrayTex->GetDesc().DepthOrArraySize;
	md3dDevice->CreateShaderResourceView(treeArrayTex.Get(), &srvDesc, hDescriptor);
}

void TreeBillboardsApp::BuildShadersAndInputLayouts()
{
	const D3D_SHADER_MACRO defines[] =
	{
		"FOG", "1",
		NULL, NULL
	};

	const D3D_SHADER_MACRO alphaTestDefines[] =
	{
		"FOG", "1",
		"ALPHA_TEST", "1",
		NULL, NULL
	};

	mShaders["standardVS"] = d3dUtil::CompileShader(L"Shaders\\Default.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["opaquePS"] = d3dUtil::CompileShader(L"Shaders\\Default.hlsl", defines, "PS", "ps_5_1");
	mShaders["alphaTestedPS"] = d3dUtil::CompileShader(L"Shaders\\Default.hlsl", alphaTestDefines, "PS", "ps_5_1");
	
	mShaders["treeSpriteVS"] = d3dUtil::CompileShader(L"Shaders\\TreeSprite.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["treeSpriteGS"] = d3dUtil::CompileShader(L"Shaders\\TreeSprite.hlsl", nullptr, "GS", "gs_5_1");
	mShaders["treeSpritePS"] = d3dUtil::CompileShader(L"Shaders\\TreeSprite.hlsl", alphaTestDefines, "PS", "ps_5_1");

    mStdInputLayout =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

	mTreeSpriteInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "SIZE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
}

void TreeBillboardsApp::BuildLandGeometry()
{
	//land creation
    GeometryGenerator geoGen;
    GeometryGenerator::MeshData grid = geoGen.CreateGrid(125.0f, 125.0f, 50, 50);
	
    std::vector<Vertex> vertices(grid.Vertices.size());
    for(size_t i = 0; i < grid.Vertices.size(); ++i)
    {
		auto& p = grid.Vertices[i].Position;
		vertices[i].Pos.x = -p.z; //rotate 90 degreees counter clockwise
		vertices[i].Pos.z = p.x; //define the z coordinate

		// Calculate the center of the grid aka 0
		float centerX = 0;
		float centerZ = 0;

		// Calculate the border size - CHOOSE 45
		float borderSize = 45.0f;

		//Height adjustment - checking to make sure the vertex is within a certain POS. 
		//as long as it is within the range of 45 keep it 2
		//otherwise dip down to -10
        if (p.x > centerX - borderSize && p.x < centerX + borderSize && 
            p.z > centerZ - borderSize && p.z < centerZ + borderSize)
        {
            // Inside the border
            vertices[i].Pos.y = 2;
        }
        else
        {
            // Outside the border
            vertices[i].Pos.y = -10;
        }
		vertices[i].Normal = GetHillsNormal(p.x, p.z);
		vertices[i].TexC = grid.Vertices[i].TexC;
      
    }

    const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);

    std::vector<std::uint16_t> indices = grid.GetIndices16();
    const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "landGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	geo->DrawArgs["grid"] = submesh;

	mGeometries["landGeo"] = std::move(geo);
}

void TreeBillboardsApp::BuildWavesGeometry()
{
    std::vector<std::uint16_t> indices(3 * mWaves->TriangleCount()); // 3 indices per face
	assert(mWaves->VertexCount() < 0x0000ffff);

    // Iterate over each quad.
    int m = mWaves->RowCount();
    int n = mWaves->ColumnCount();
    int k = 0;
    for(int i = 0; i < m - 1; ++i)
    {
        for(int j = 0; j < n - 1; ++j)
        {
            indices[k] = i*n + j;
            indices[k + 1] = i*n + j + 1;
            indices[k + 2] = (i + 1)*n + j;

            indices[k + 3] = (i + 1)*n + j;
            indices[k + 4] = i*n + j + 1;
            indices[k + 5] = (i + 1)*n + j + 1;

            k += 6; // next quad
        }
    }

	UINT vbByteSize = mWaves->VertexCount()*sizeof(Vertex);
	UINT ibByteSize = (UINT)indices.size()*sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "waterGeo";

	// Set dynamically.
	geo->VertexBufferCPU = nullptr;
	geo->VertexBufferGPU = nullptr;

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	geo->DrawArgs["grid"] = submesh;

	mGeometries["waterGeo"] = std::move(geo);
}

//create the new shapes in here
void TreeBillboardsApp::BuildBoxGeometry()
{
	//DEClARE SHAPES
	GeometryGenerator geoGen;
	//box shape - 1 
	GeometryGenerator::MeshData box = geoGen.CreateBox(4.5f, 3.5f, 4.5f, 3);
	//cylinder - 3
	GeometryGenerator::MeshData cylinder = geoGen.CreateCylinder(0.5f, 0.3f, 6.0f, 20, 20);
	//sphere - 4
	GeometryGenerator::MeshData sphere = geoGen.CreateSphere(0.5f, 20, 20);
	//geosphere - 5 
	GeometryGenerator::MeshData GEOsphere = geoGen.CreateGeosphere(1.0f, 2);
	//quad - 6
	GeometryGenerator::MeshData quad = geoGen.CreateQuad(1.0f, 1.0f, 1.0f, 1.0f, 0.5f);
	//triangular prism - 7
	GeometryGenerator::MeshData triPrism = geoGen.CreateTriangularPrism(1.0f, 1.0f, 3);
	//cone - 8
	GeometryGenerator::MeshData cone = geoGen.CreateCone(1.0f, 1.0f, 9, 5);
	//pyramid - 9
	GeometryGenerator::MeshData pyramid = geoGen.CreatePyramid(1.0f, 1.0f, 5);
	//diamond - 10
	GeometryGenerator::MeshData diamond = geoGen.CreateDiamond(1.0f, 1.0f, 1.0, 2);
	//wedge - 11
	GeometryGenerator::MeshData wedge = geoGen.CreateWedge(1.0f, 1.0f, 1.0, 2);
	//torus -12
	GeometryGenerator::MeshData torus = geoGen.CreateTorus(2.0f, 0.5f, 20, 20);

	//-----------------------------//
	
	//Vertex Cashe
	UINT boxVertexOffset = 0;
	UINT cylinderVertexOffset = boxVertexOffset + (UINT)box.Vertices.size();
	UINT sphereVertexOffset = cylinderVertexOffset + (UINT)cylinder.Vertices.size();
	UINT geoSphereVertexOffset = sphereVertexOffset + (UINT)sphere.Vertices.size();
	UINT quadVertexOffset = geoSphereVertexOffset + (UINT)GEOsphere.Vertices.size();
	UINT triPrismVertexOffset = quadVertexOffset + (UINT)quad.Vertices.size();
	UINT coneVertexOffset = triPrismVertexOffset + (UINT)triPrism.Vertices.size();
	UINT pyramidVertexOffset = coneVertexOffset + (UINT)cone.Vertices.size();
	UINT diamondVertexOffset = pyramidVertexOffset + (UINT)pyramid.Vertices.size();
	UINT wedgeVertexOffset = diamondVertexOffset + (UINT)diamond.Vertices.size();
	UINT torusVertexOffset = wedgeVertexOffset + (UINT)wedge.Vertices.size();

	//Index Cashe
	UINT boxIndexOffset = 0;
	UINT cylinderIndexOffset = boxIndexOffset + (UINT)box.Indices32.size();
	UINT sphereIndexOffset = cylinderIndexOffset + (UINT)cylinder.Indices32.size();
	UINT geoSphereIndexOffset = sphereIndexOffset + (UINT)sphere.Indices32.size();
	UINT quadIndexOffset = geoSphereIndexOffset + (UINT)GEOsphere.Indices32.size();
	UINT triPrismIndexOffset = quadIndexOffset + (UINT)quad.Indices32.size();
	UINT coneIndexOffset = triPrismIndexOffset + (UINT)triPrism.Indices32.size();
	UINT pyramidIndexOffset = coneIndexOffset + (UINT)cone.Indices32.size();
	UINT diamondIndexOffset = pyramidIndexOffset + (UINT)pyramid.Indices32.size();
	UINT wedgeIndexOffset = diamondIndexOffset + (UINT)diamond.Indices32.size();
	UINT torusIndexOffset = wedgeIndexOffset + (UINT)wedge.Indices32.size();

	//adding submesh
	//box
	SubmeshGeometry boxSubmesh;
	boxSubmesh.IndexCount = (UINT)box.Indices32.size();
	boxSubmesh.StartIndexLocation = boxIndexOffset;
	boxSubmesh.BaseVertexLocation = boxVertexOffset;
	//cylinder
	SubmeshGeometry cylinderSubmesh;
	cylinderSubmesh.IndexCount = (UINT)cylinder.Indices32.size();
	cylinderSubmesh.StartIndexLocation = cylinderIndexOffset;
	cylinderSubmesh.BaseVertexLocation = cylinderVertexOffset;
	//sphere
	SubmeshGeometry sphereSubmesh;
	sphereSubmesh.IndexCount = (UINT)sphere.Indices32.size();
	sphereSubmesh.StartIndexLocation = sphereIndexOffset;
	sphereSubmesh.BaseVertexLocation = sphereVertexOffset;
	//geosphere
	SubmeshGeometry geoSphereSubmesh;
	geoSphereSubmesh.IndexCount = (UINT)GEOsphere.Indices32.size();
	geoSphereSubmesh.StartIndexLocation = geoSphereIndexOffset;
	geoSphereSubmesh.BaseVertexLocation = geoSphereVertexOffset;
	//quad
	SubmeshGeometry quadSubmesh;
	quadSubmesh.IndexCount = (UINT)quad.Indices32.size();
	quadSubmesh.StartIndexLocation = quadIndexOffset;
	quadSubmesh.BaseVertexLocation = quadVertexOffset;
	//tri prism
	SubmeshGeometry triPrismSubmesh;
	triPrismSubmesh.IndexCount = (UINT)triPrism.Indices32.size();
	triPrismSubmesh.StartIndexLocation = triPrismIndexOffset;
	triPrismSubmesh.BaseVertexLocation = triPrismVertexOffset;
	//cone
	SubmeshGeometry coneSubmesh;
	coneSubmesh.IndexCount = (UINT)cone.Indices32.size();
	coneSubmesh.StartIndexLocation = coneIndexOffset;
	coneSubmesh.BaseVertexLocation = coneVertexOffset;
	//pyramid
	SubmeshGeometry pyramidSubmesh;
	pyramidSubmesh.IndexCount = (UINT)pyramid.Indices32.size();
	pyramidSubmesh.StartIndexLocation = pyramidIndexOffset;
	pyramidSubmesh.BaseVertexLocation = pyramidVertexOffset;
	//diamond
	SubmeshGeometry diamondSubmesh;
	diamondSubmesh.IndexCount = (UINT)diamond.Indices32.size();
	diamondSubmesh.StartIndexLocation = diamondIndexOffset;
	diamondSubmesh.BaseVertexLocation = diamondVertexOffset;
	//wedge
	SubmeshGeometry wedgeSubmesh;
	wedgeSubmesh.IndexCount = (UINT)wedge.Indices32.size();
	wedgeSubmesh.StartIndexLocation = wedgeIndexOffset;
	wedgeSubmesh.BaseVertexLocation = wedgeVertexOffset;
	//torus
	SubmeshGeometry torusSubmesh;
	torusSubmesh.IndexCount = (UINT)torus.Indices32.size();
	torusSubmesh.StartIndexLocation = torusIndexOffset;
	torusSubmesh.BaseVertexLocation = torusVertexOffset;

	//total vertex count
	auto totalVertexCount =
		box.Vertices.size() +
		cylinder.Vertices.size() +
		sphere.Vertices.size() +
		GEOsphere.Vertices.size() +
		quad.Vertices.size() +
		triPrism.Vertices.size() +
		cone.Vertices.size() +
		pyramid.Vertices.size() +
		diamond.Vertices.size() +
		wedge.Vertices.size() +
		torus.Vertices.size();


	std::vector<Vertex> vertices(totalVertexCount); //array
	UINT k = 0;
	for (size_t i = 0; i < box.Vertices.size(); ++i, ++k)
	{
		auto& p = box.Vertices[i].Position;
		vertices[k].Pos = p;
		vertices[k].Normal = box.Vertices[i].Normal;
		vertices[k].TexC = box.Vertices[i].TexC;
	}
	for (size_t i = 0; i < cylinder.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = cylinder.Vertices[i].Position;
		vertices[k].Normal = cylinder.Vertices[i].Normal;
		vertices[k].TexC = cylinder.Vertices[i].TexC;
		//vertices[k].Color = XMFLOAT4(DirectX::Colors::SteelBlue);
	}
	for (size_t i = 0; i < sphere.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = sphere.Vertices[i].Position;
		vertices[k].Normal = sphere.Vertices[i].Normal;
		vertices[k].TexC = sphere.Vertices[i].TexC;
		//vertices[k].Color = XMFLOAT4(DirectX::Colors::DarkOliveGreen);
	}
	for (size_t i = 0; i < GEOsphere.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = GEOsphere.Vertices[i].Position;
		vertices[k].Normal = GEOsphere.Vertices[i].Normal;
		vertices[k].TexC = GEOsphere.Vertices[i].TexC;
		//vertices[k].Color = XMFLOAT4(DirectX::Colors::Orange);
	}
	for (size_t i = 0; i < quad.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = quad.Vertices[i].Position;
		vertices[k].Normal = quad.Vertices[i].Normal;
		vertices[k].TexC = quad.Vertices[i].TexC;
		//vertices[k].Color = XMFLOAT4(DirectX::Colors::DarkOliveGreen);
	}
	for (size_t i = 0; i < triPrism.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = triPrism.Vertices[i].Position;
		vertices[k].Normal = triPrism.Vertices[i].Normal;
		vertices[k].TexC = triPrism.Vertices[i].TexC;
		//vertices[k].Color = XMFLOAT4(DirectX::Colors::DarkSlateGray);
	}
	for (size_t i = 0; i < cone.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = cone.Vertices[i].Position;
		vertices[k].Normal = cone.Vertices[i].Normal;
		vertices[k].TexC = cone.Vertices[i].TexC;
		//vertices[k].Color = XMFLOAT4(DirectX::Colors::DimGray);
	}
	for (size_t i = 0; i < pyramid.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = pyramid.Vertices[i].Position;
		vertices[k].Normal = pyramid.Vertices[i].Normal;
		vertices[k].TexC = pyramid.Vertices[i].TexC;
		//vertices[k].Color = XMFLOAT4(DirectX::Colors::DarkOliveGreen);
	}
	for (size_t i = 0; i < diamond.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = diamond.Vertices[i].Position;
		vertices[k].Normal = diamond.Vertices[i].Normal;
		vertices[k].TexC = diamond.Vertices[i].TexC;
		//vertices[k].Color = XMFLOAT4(DirectX::Colors::CornflowerBlue);
	}
	for (size_t i = 0; i < wedge.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = wedge.Vertices[i].Position;
		vertices[k].Normal = wedge.Vertices[i].Normal;
		vertices[k].TexC = wedge.Vertices[i].TexC;
		//vertices[k].Color = XMFLOAT4(DirectX::Colors::DimGray);
	}
	for (size_t i = 0; i < torus.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = torus.Vertices[i].Position;
		vertices[k].Normal = torus.Vertices[i].Normal;
		vertices[k].TexC = torus.Vertices[i].TexC;
		//vertices[k].Color = XMFLOAT4(DirectX::Colors::DarkOliveGreen);
	}

	//inserting the indices
	std::vector<std::uint16_t> indices;
	indices.insert(indices.end(), std::begin(box.GetIndices16()), std::end(box.GetIndices16()));
	indices.insert(indices.end(), std::begin(cylinder.GetIndices16()), std::end(cylinder.GetIndices16()));
	indices.insert(indices.end(), std::begin(sphere.GetIndices16()), std::end(sphere.GetIndices16()));
	indices.insert(indices.end(), std::begin(GEOsphere.GetIndices16()), std::end(GEOsphere.GetIndices16()));
	indices.insert(indices.end(), std::begin(quad.GetIndices16()), std::end(quad.GetIndices16()));
	indices.insert(indices.end(), std::begin(triPrism.GetIndices16()), std::end(triPrism.GetIndices16()));
	indices.insert(indices.end(), std::begin(cone.GetIndices16()), std::end(cone.GetIndices16()));
	indices.insert(indices.end(), std::begin(pyramid.GetIndices16()), std::end(pyramid.GetIndices16()));
	indices.insert(indices.end(), std::begin(diamond.GetIndices16()), std::end(diamond.GetIndices16()));
	indices.insert(indices.end(), std::begin(wedge.GetIndices16()), std::end(wedge.GetIndices16()));
	indices.insert(indices.end(), std::begin(torus.GetIndices16()), std::end(torus.GetIndices16()));

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	//draw the shape
	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "boxGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	//geo drawing
	geo->DrawArgs["box"] = boxSubmesh;
	geo->DrawArgs["cylinder"] = cylinderSubmesh;
	geo->DrawArgs["sphere"] = sphereSubmesh;
	geo->DrawArgs["geosphere"] = geoSphereSubmesh;
	geo->DrawArgs["quad"] = quadSubmesh;
	geo->DrawArgs["triprism"] = triPrismSubmesh;
	geo->DrawArgs["cone"] = coneSubmesh;
	geo->DrawArgs["pyramid"] = pyramidSubmesh;
	geo->DrawArgs["diamond"] = diamondSubmesh;
	geo->DrawArgs["wedge"] = wedgeSubmesh;
	geo->DrawArgs["torus"] = torusSubmesh;

	mGeometries["boxGeo"] = std::move(geo);
}

void TreeBillboardsApp::BuildTreeSpritesGeometry()
{
	//step5
	struct TreeSpriteVertex
	{
		XMFLOAT3 Pos;
		XMFLOAT2 Size;
	};

	static const int treeCount = 4;
	std::array<TreeSpriteVertex, 16> vertices;
	for(UINT i = 0; i < treeCount; ++i)
	{
		float x = MathHelper::RandF(-30.0f, 68.0f); //for the trees
		float z = MathHelper::RandF(-45.0f, 55.0f); //for the trees
		float y = GetHillsHeight(x, z);

		// Move tree slightly above land height.
		y = 10.0f;

		vertices[i].Pos = XMFLOAT3(x, y, z);
		vertices[i].Size = XMFLOAT2(20.0f, 20.0f);
	}

	std::array<std::uint16_t, 16> indices =
	{
		0, 1, 2, 3, 4, 5, 6, 7,
		8, 9, 10, 11, 12, 13, 14, 15
	};

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(TreeSpriteVertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "treeSpritesGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(TreeSpriteVertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	geo->DrawArgs["points"] = submesh;

	mGeometries["treeSpritesGeo"] = std::move(geo);
}

void TreeBillboardsApp::BuildPSOs()
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;

	//
	// PSO for opaque objects.
	//
    ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	opaquePsoDesc.InputLayout = { mStdInputLayout.data(), (UINT)mStdInputLayout.size() };
	opaquePsoDesc.pRootSignature = mRootSignature.Get();
	opaquePsoDesc.VS = 
	{ 
		reinterpret_cast<BYTE*>(mShaders["standardVS"]->GetBufferPointer()), 
		mShaders["standardVS"]->GetBufferSize()
	};
	opaquePsoDesc.PS = 
	{ 
		reinterpret_cast<BYTE*>(mShaders["opaquePS"]->GetBufferPointer()),
		mShaders["opaquePS"]->GetBufferSize()
	};
	opaquePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	opaquePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	opaquePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	opaquePsoDesc.SampleMask = UINT_MAX;
	opaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	opaquePsoDesc.NumRenderTargets = 1;
	opaquePsoDesc.RTVFormats[0] = mBackBufferFormat;

	//there is abug with F2 key that is supposed to turn on the multisampling!
//Set4xMsaaState(true);
	//m4xMsaaState = true;

	opaquePsoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	opaquePsoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	opaquePsoDesc.DSVFormat = mDepthStencilFormat;
    ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&mPSOs["opaque"])));

	//
	// PSO for transparent objects
	//

	D3D12_GRAPHICS_PIPELINE_STATE_DESC transparentPsoDesc = opaquePsoDesc;

	D3D12_RENDER_TARGET_BLEND_DESC transparencyBlendDesc;
	transparencyBlendDesc.BlendEnable = true;
	transparencyBlendDesc.LogicOpEnable = false;
	transparencyBlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
	transparencyBlendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	transparencyBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
	transparencyBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
	transparencyBlendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
	transparencyBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	transparencyBlendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
	transparencyBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	//transparentPsoDesc.BlendState.AlphaToCoverageEnable = true;

	transparentPsoDesc.BlendState.RenderTarget[0] = transparencyBlendDesc;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&transparentPsoDesc, IID_PPV_ARGS(&mPSOs["transparent"])));

	//
	// PSO for alpha tested objects
	//

	D3D12_GRAPHICS_PIPELINE_STATE_DESC alphaTestedPsoDesc = opaquePsoDesc;
	alphaTestedPsoDesc.PS = 
	{ 
		reinterpret_cast<BYTE*>(mShaders["alphaTestedPS"]->GetBufferPointer()),
		mShaders["alphaTestedPS"]->GetBufferSize()
	};
	alphaTestedPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&alphaTestedPsoDesc, IID_PPV_ARGS(&mPSOs["alphaTested"])));

	//
	// PSO for tree sprites
	//
	D3D12_GRAPHICS_PIPELINE_STATE_DESC treeSpritePsoDesc = opaquePsoDesc;
	treeSpritePsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["treeSpriteVS"]->GetBufferPointer()),
		mShaders["treeSpriteVS"]->GetBufferSize()
	};
	treeSpritePsoDesc.GS =
	{
		reinterpret_cast<BYTE*>(mShaders["treeSpriteGS"]->GetBufferPointer()),
		mShaders["treeSpriteGS"]->GetBufferSize()
	};
	treeSpritePsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["treeSpritePS"]->GetBufferPointer()),
		mShaders["treeSpritePS"]->GetBufferSize()
	};
	//step1
	treeSpritePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	treeSpritePsoDesc.InputLayout = { mTreeSpriteInputLayout.data(), (UINT)mTreeSpriteInputLayout.size() };
	treeSpritePsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&treeSpritePsoDesc, IID_PPV_ARGS(&mPSOs["treeSprites"])));
}

void TreeBillboardsApp::BuildFrameResources()
{
    for(int i = 0; i < gNumFrameResources; ++i)
    {
        mFrameResources.push_back(std::make_unique<FrameResource>(md3dDevice.Get(),
            1, (UINT)mAllRitems.size(), (UINT)mMaterials.size(), mWaves->VertexCount()));
    }
}

void TreeBillboardsApp::BuildMaterials()
{
	auto grass = std::make_unique<Material>();
	grass->Name = "grass";
	grass->MatCBIndex = 0;
	grass->DiffuseSrvHeapIndex = 0;
	grass->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	grass->FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
	grass->Roughness = 0.125f;

	// This is not a good water material definition, but we do not have all the rendering
	// tools we need (transparency, environment reflection), so we fake it for now.
	auto water = std::make_unique<Material>();
	water->Name = "water";
	water->MatCBIndex = 1;
	water->DiffuseSrvHeapIndex = 1;
	water->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.5f);
	water->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	water->Roughness = 0.0f;

	auto wirefence = std::make_unique<Material>();
	wirefence->Name = "wirefence";
	wirefence->MatCBIndex = 2;
	wirefence->DiffuseSrvHeapIndex = 2;
	wirefence->DiffuseAlbedo = XMFLOAT4(Colors::LightSteelBlue);
	wirefence->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
	wirefence->Roughness = 0.3f;

	//add new mats
	//stone materials
	auto stone = std::make_unique<Material>();
	stone->Name = "stone";
	stone->MatCBIndex = 3;
	stone->DiffuseSrvHeapIndex = 3;
	stone->DiffuseAlbedo = XMFLOAT4(Colors::LightSteelBlue);
	stone->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
	stone->Roughness = 0.3f;

	//marble materials
	auto marble = std::make_unique<Material>();
	marble->Name = "marble";
	marble->MatCBIndex = 4;
	marble->DiffuseSrvHeapIndex = 4;
	marble->DiffuseAlbedo = XMFLOAT4(Colors::LightSteelBlue);
	marble->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
	marble->Roughness = 0.3f;

	//sun materials
	auto sun = std::make_unique<Material>();
	sun->Name = "sun";
	sun->MatCBIndex = 5;
	sun->DiffuseSrvHeapIndex = 5;
	sun->DiffuseAlbedo = XMFLOAT4(Colors::LightSteelBlue);
	sun->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
	sun->Roughness = 0.3f;

	//diamond materials
	auto diamond = std::make_unique<Material>();
	diamond->Name = "diamond";
	diamond->MatCBIndex = 6;
	diamond->DiffuseSrvHeapIndex = 6;
	diamond->DiffuseAlbedo = XMFLOAT4(Colors::LightSteelBlue);
	diamond->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
	diamond->Roughness = 0.3f;

	//bush mats
	auto bush = std::make_unique<Material>();
	bush->Name = "bush";
	bush->MatCBIndex = 7;
	bush->DiffuseSrvHeapIndex = 7;
	bush->DiffuseAlbedo = XMFLOAT4(Colors::LightSteelBlue);
	bush->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
	bush->Roughness = 0.3f;

	//wood
	auto wood = std::make_unique<Material>();
	wood->Name = "wood";
	wood->MatCBIndex = 8;
	wood->DiffuseSrvHeapIndex = 8;
	wood->DiffuseAlbedo = XMFLOAT4(Colors::LightSteelBlue);
	wood->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
	wood->Roughness = 0.3f;

	//leave tree last
	auto treeSprites = std::make_unique<Material>();
	treeSprites->Name = "treeSprites";
	treeSprites->MatCBIndex = 9;
	treeSprites->DiffuseSrvHeapIndex = 9;
	treeSprites->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	treeSprites->FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
	treeSprites->Roughness = 0.125f;

	mMaterials["grass"] = std::move(grass);
	mMaterials["water"] = std::move(water);
	mMaterials["wirefence"] = std::move(wirefence);
	mMaterials["stone"] = std::move(stone);
	mMaterials["marble"] = std::move(marble);
	mMaterials["sun"] = std::move(sun);
	mMaterials["diamond"] = std::move(diamond);
	mMaterials["bush"] = std::move(bush);
	mMaterials["wood"] = std::move(wood);
	mMaterials["treeSprites"] = std::move(treeSprites);

}

void TreeBillboardsApp::BuildRenderItems()
{
	UINT objCBIndex = 0;

    auto wavesRitem = std::make_unique<RenderItem>();
    wavesRitem->World = MathHelper::Identity4x4();
	XMStoreFloat4x4(&wavesRitem->TexTransform, XMMatrixScaling(5.0f, 5.0f, 1.0f));
	wavesRitem->ObjCBIndex = objCBIndex;
	wavesRitem->Mat = mMaterials["water"].get();
	wavesRitem->Geo = mGeometries["waterGeo"].get();
	wavesRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	wavesRitem->IndexCount = wavesRitem->Geo->DrawArgs["grid"].IndexCount;
	wavesRitem->StartIndexLocation = wavesRitem->Geo->DrawArgs["grid"].StartIndexLocation;
	wavesRitem->BaseVertexLocation = wavesRitem->Geo->DrawArgs["grid"].BaseVertexLocation;

    mWavesRitem = wavesRitem.get();

	mRitemLayer[(int)RenderLayer::Transparent].push_back(wavesRitem.get());

    auto gridRitem = std::make_unique<RenderItem>();
    gridRitem->World = MathHelper::Identity4x4();
	XMStoreFloat4x4(&gridRitem->TexTransform, XMMatrixScaling(5.0f, 5.0f, 1.0f));
	objCBIndex++;
	gridRitem->ObjCBIndex = objCBIndex;
	gridRitem->Mat = mMaterials["grass"].get();
	gridRitem->Geo = mGeometries["landGeo"].get();
	gridRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    gridRitem->IndexCount = gridRitem->Geo->DrawArgs["grid"].IndexCount;
    gridRitem->StartIndexLocation = gridRitem->Geo->DrawArgs["grid"].StartIndexLocation;
    gridRitem->BaseVertexLocation = gridRitem->Geo->DrawArgs["grid"].BaseVertexLocation;

	mRitemLayer[(int)RenderLayer::Opaque].push_back(gridRitem.get());

	//building the objects here

	//quad - DOOR
	objCBIndex++;
	CreateNewObject("quad", XMMatrixRotationAxis({ 1,0,0,0 }, XMConvertToRadians(90)) * XMMatrixScaling(30.0f, 15.5f, 62.0f),
		XMMatrixTranslation(-45.0f, 10.0f, -30.0f), 
		XMMatrixRotationRollPitchYaw(0.f, 0.f, 0.f),
		objCBIndex, "stone");

	//quad - Floor
	objCBIndex++;
	CreateNewObject("quad", XMMatrixScaling(15.0f, 15.5f, 15.0f),
		XMMatrixTranslation(-22.0f, 1.0f, -39.0f),
		XMMatrixRotationRollPitchYaw(0.f, 0.f, 0.f),
		objCBIndex, "stone");

	//tri prism - TOMB
	objCBIndex++;
	CreateNewObject("triprism", XMMatrixRotationAxis({ 0,0,1,0 }, XMConvertToRadians(90)) * XMMatrixScaling(15.0f, 3.0f, 3.0f),
		XMMatrixTranslation(0.0f, 5.5f, 18.0f),
		XMMatrixRotationRollPitchYaw(0.f, 0.f, 0.f),
		objCBIndex, "stone");

	objCBIndex++;
	CreateNewObject("box", XMMatrixScaling(3.7f, 1.2f, 1.2f),
		XMMatrixTranslation(0.0f, 2.5f, 18.0f),
		XMMatrixRotationRollPitchYaw(0.f, 0.f, 0.f),
		objCBIndex, "marble");

	//pyramid - BUSH
	objCBIndex++;
	CreateNewObject("pyramid", XMMatrixScaling(4.0f, 4.0f, 4.0f),
		XMMatrixTranslation(-32.0f, 9.0f, -22.0f),
		XMMatrixRotationRollPitchYaw(0.f, 0.f, 0.f),
		objCBIndex, "bush");

	objCBIndex++;
	CreateNewObject("cylinder", XMMatrixScaling(1.5f, 1.5f, 1.5f),
		XMMatrixTranslation(-32.0f, 6.0f, -22.0f),
		XMMatrixRotationRollPitchYaw(0.f, 0.f, 0.f),
		objCBIndex, "wood");

	//pyramid - BUSH 2
	objCBIndex++;
	CreateNewObject("pyramid", XMMatrixScaling(4.0f, 4.0f, 4.0f),
		XMMatrixTranslation(32.0f, 9.0f, -22.0f),
		XMMatrixRotationRollPitchYaw(0.f, 0.f, 0.f),
		objCBIndex, "bush");

	objCBIndex++;
	CreateNewObject("cylinder", XMMatrixScaling(1.5f, 1.5f, 1.5f),
		XMMatrixTranslation(32.0f, 6.0f, -22.0f),
		XMMatrixRotationRollPitchYaw(0.f, 0.f, 0.f),
		objCBIndex, "wood");

	//geosphere
	objCBIndex++;
	CreateNewObject("geosphere", XMMatrixScaling(9.0f, 9.0f, 9.0f),
		XMMatrixTranslation(-25.0f, 35.0f, 100.0f),  
		XMMatrixRotationRollPitchYaw(0.f, 0.f, 0.f),
		objCBIndex, "sun");

	//diamond
	objCBIndex++;
	CreateNewObject("diamond", XMMatrixScaling(4.0f, 4.0f, 4.0f),
		XMMatrixTranslation(0.0f, 10.0f, 18.0f),
		XMMatrixRotationRollPitchYaw(0.f, 0.f, 0.f),
		objCBIndex, "diamond");

	//BUILDING WALLS FOR MONUMENT
	objCBIndex++;
	CreateNewObject("box", XMMatrixScaling(7.0f, 7.0f, 0.5f),
		XMMatrixTranslation(0.0f, 6.5f, 30.0f),
		XMMatrixRotationRollPitchYaw(0.f, 0.f, 0.f),
		objCBIndex, "marble");

	objCBIndex++;
	CreateNewObject("box", XMMatrixScaling(7.0f, 7.0f, 0.5f),
		XMMatrixTranslation(0.0f, 6.5f, -30.0f),
		XMMatrixRotationRollPitchYaw(0.f, 0.f, 0.f),
		objCBIndex, "marble");

	objCBIndex++;
	CreateNewObject("box", XMMatrixScaling(0.5f, 7.0f, 13.0f),
		XMMatrixTranslation(15.0f, 6.5f, 0.0f),
		XMMatrixRotationRollPitchYaw(0.f, 0.f, 0.f),
		objCBIndex, "marble");

	objCBIndex++;
	CreateNewObject("box", XMMatrixScaling(0.5f, 7.0f, 13.0f),
		XMMatrixTranslation(-15.0f, 6.5f, 0.0f),
		XMMatrixRotationRollPitchYaw(0.f, 0.f, 0.f),
		objCBIndex, "marble");

	//tops for the pillars
	objCBIndex++;
	CreateNewObject("box", XMMatrixScaling(10.3f, 0.2f, 0.5f),
		XMMatrixTranslation(0.0f, 22.5f, 30.0f),
		XMMatrixRotationRollPitchYaw(0.f, 0.f, 0.f),
		objCBIndex, "marble");

	objCBIndex++;
	CreateNewObject("box", XMMatrixScaling(10.3f, 0.2f, 0.5f),
		XMMatrixTranslation(0.0f, 22.5f, -30.0f),
		XMMatrixRotationRollPitchYaw(0.f, 0.f, 0.f),
		objCBIndex, "marble");

	objCBIndex++;
	CreateNewObject("box", XMMatrixScaling(0.5f, 0.2f, 13.0f),
		XMMatrixTranslation(22.0f, 22.5f, 0.0f),
		XMMatrixRotationRollPitchYaw(0.f, 0.f, 0.f),
		objCBIndex, "marble");

	objCBIndex++;
	CreateNewObject("box", XMMatrixScaling(0.5f, 0.2f, 13.0f),
		XMMatrixTranslation(-22.0f, 22.5f, 0.0f),
		XMMatrixRotationRollPitchYaw(0.f, 0.f, 0.f),
		objCBIndex, "marble");

	//bottoms for the pillars
	objCBIndex++;
	CreateNewObject("box", XMMatrixScaling(10.3f, 0.2f, 0.5f),
		XMMatrixTranslation(0.0f, 2.5f, 32.0f),
		XMMatrixRotationRollPitchYaw(0.f, 0.f, 0.f),
		objCBIndex, "marble");

	objCBIndex++;
	CreateNewObject("box", XMMatrixScaling(10.3f, 0.2f, 0.5f),
		XMMatrixTranslation(0.0f, 2.5f, -32.0f),
		XMMatrixRotationRollPitchYaw(0.f, 0.f, 0.f),
		objCBIndex, "marble");

	objCBIndex++;
	CreateNewObject("box", XMMatrixScaling(0.5f, 0.2f, 14.0f),
		XMMatrixTranslation(22.0f, 2.5f, 0.0f),
		XMMatrixRotationRollPitchYaw(0.f, 0.f, 0.f),
		objCBIndex, "marble");

	objCBIndex++;
	CreateNewObject("box", XMMatrixScaling(0.5f, 0.2f, 14.0f),
		XMMatrixTranslation(-22.0f, 2.5f, 0.0f),
		XMMatrixRotationRollPitchYaw(0.f, 0.f, 0.f),
		objCBIndex, "marble");

	// CLOCK TOWER
	objCBIndex++;
	CreateNewObject("box", XMMatrixScaling(3.0f, 10.0f, 2.0f),
		XMMatrixTranslation(0.0f, 18.0f, 0.0f),
		XMMatrixRotationRollPitchYaw(0.f, 0.f, 0.f),
		objCBIndex, "marble"); 
	objCBIndex++;
	CreateNewObject("cone", XMMatrixScaling(10.0f, 10.0f, 10.0f),
		XMMatrixTranslation(0.0f, 40.0f, 0.0f),
		XMMatrixRotationRollPitchYaw(0.f, 0.f, 0.f),
		objCBIndex, "stone");

	objCBIndex++;
	CreateNewObject("sphere", XMMatrixScaling(11.0f, 11.0f, 11.0f),
		XMMatrixTranslation(0.0f, 25.0f, 0.0f),  
		XMMatrixRotationRollPitchYaw(0.f, 0.f, 0.f),
		objCBIndex, "stone");

	//torus - WHY WONT YOU CHANGE MAT
	objCBIndex++;
	CreateNewObject("torus", XMMatrixRotationAxis({ 1,0,0,0 }, XMConvertToRadians(90)) * XMMatrixScaling(2.5f, 2.5f, 2.5f),
		XMMatrixTranslation(0.0f, 25.0f, -4.0f),
		XMMatrixRotationRollPitchYaw(0.f, 0.f, 0.f),
		objCBIndex, "stone");

	objCBIndex++;
	CreateNewObject("torus", XMMatrixRotationAxis({ 1,0,0,0 }, XMConvertToRadians(90))* XMMatrixScaling(2.5f, 2.5f, 2.5f),
		XMMatrixTranslation(0.0f, 25.0f, 4.0f),
		XMMatrixRotationRollPitchYaw(0.f, 0.f, 0.f),
		objCBIndex, "stone");


	//add the for loop for the colomns, wedges and spheres
	for (int i = 0; i < 6; ++i)
	{
		//right cylinder
		objCBIndex++;
		CreateNewObject("cylinder", XMMatrixScaling(4.0f, 4.5f, 4.0f),
			XMMatrixTranslation(22.0f, 10.0f, -25.0f + i * 10.0f),
			XMMatrixRotationRollPitchYaw(0.f, 0.f, 0.f),
			objCBIndex, "marble");

		//left cylinder
		objCBIndex++;
		CreateNewObject("cylinder", XMMatrixScaling(4.0f, 4.5f, 4.0f),
			XMMatrixTranslation(-22.0f, 10.0f, -25.0f + i * 10.0f), 
			XMMatrixRotationRollPitchYaw(0.f, 0.f, 0.f),
			objCBIndex, "marble");

		//right wedge
		objCBIndex++;
		CreateNewObject("wedge", XMMatrixRotationAxis({ 0,1,0,0 }, XMConvertToRadians(270)) * XMMatrixScaling(3.5f, 4.0f, 3.5f),
			XMMatrixTranslation(24.0f, 4.0f, -25.0f + i * 10.0f),
			XMMatrixRotationRollPitchYaw(0.f, 0.f, 0.f),
			objCBIndex, "marble");

		//left wedge
		objCBIndex++;
		CreateNewObject("wedge", XMMatrixRotationAxis({ 0,1,0,0 }, XMConvertToRadians(90)) * XMMatrixScaling(3.5f, 4.0f, 3.5f),
			XMMatrixTranslation(-24.0f, 4.0f, -25.0f + i * 10.0f),
			XMMatrixRotationRollPitchYaw(0.f, 0.f, 0.f),
			objCBIndex, "marble");

		//right cone
		objCBIndex++;
		CreateNewObject("cone", XMMatrixScaling(3.0f, 3.5f, 3.0f),
			XMMatrixTranslation(22.0f, 25.0f, -25.0f + i * 10.0f),
			XMMatrixRotationRollPitchYaw(0.f, 0.f, 0.f),
			objCBIndex, "marble");

		//left cone
		objCBIndex++;
		CreateNewObject("cone", XMMatrixScaling(3.0f, 3.5f, 3.0f),
			XMMatrixTranslation(-22.0f, 25.0f, -25.0f + i * 10.0f),
			XMMatrixRotationRollPitchYaw(0.f, 0.f, 0.f),
			objCBIndex, "marble");

	}

	auto treeSpritesRitem = std::make_unique<RenderItem>();
	treeSpritesRitem->World = MathHelper::Identity4x4();
	objCBIndex++;
	treeSpritesRitem->ObjCBIndex = objCBIndex;
	treeSpritesRitem->Mat = mMaterials["treeSprites"].get();
	treeSpritesRitem->Geo = mGeometries["treeSpritesGeo"].get();
	//step2
	treeSpritesRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
	treeSpritesRitem->IndexCount = treeSpritesRitem->Geo->DrawArgs["points"].IndexCount;
	treeSpritesRitem->StartIndexLocation = treeSpritesRitem->Geo->DrawArgs["points"].StartIndexLocation;
	treeSpritesRitem->BaseVertexLocation = treeSpritesRitem->Geo->DrawArgs["points"].BaseVertexLocation;

	mRitemLayer[(int)RenderLayer::AlphaTestedTreeSprites].push_back(treeSpritesRitem.get());


    mAllRitems.push_back(std::move(wavesRitem));
    mAllRitems.push_back(std::move(gridRitem));
	mAllRitems.push_back(std::move(treeSpritesRitem));
}

void TreeBillboardsApp::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems)
{
    UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
    UINT matCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(MaterialConstants));

	auto objectCB = mCurrFrameResource->ObjectCB->Resource();
	auto matCB = mCurrFrameResource->MaterialCB->Resource();

    // For each render item...
    for(size_t i = 0; i < ritems.size(); ++i)
    {
        auto ri = ritems[i];

        cmdList->IASetVertexBuffers(0, 1, &ri->Geo->VertexBufferView());
        cmdList->IASetIndexBuffer(&ri->Geo->IndexBufferView());
		//step3
        cmdList->IASetPrimitiveTopology(ri->PrimitiveType);

		CD3DX12_GPU_DESCRIPTOR_HANDLE tex(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		tex.Offset(ri->Mat->DiffuseSrvHeapIndex, mCbvSrvDescriptorSize);

        D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + ri->ObjCBIndex*objCBByteSize;
		D3D12_GPU_VIRTUAL_ADDRESS matCBAddress = matCB->GetGPUVirtualAddress() + ri->Mat->MatCBIndex*matCBByteSize;

		cmdList->SetGraphicsRootDescriptorTable(0, tex);
        cmdList->SetGraphicsRootConstantBufferView(1, objCBAddress);
        cmdList->SetGraphicsRootConstantBufferView(3, matCBAddress);

        cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
    }
}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> TreeBillboardsApp::GetStaticSamplers()
{
	// Applications usually only need a handful of samplers.  So just define them all up front
	// and keep them available as part of the root signature.  

	const CD3DX12_STATIC_SAMPLER_DESC pointWrap(
		0, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
		1, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
		2, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
		3, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
		4, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressW
		0.0f,                             // mipLODBias
		8);                               // maxAnisotropy

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(
		5, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressW
		0.0f,                              // mipLODBias
		8);                                // maxAnisotropy

	return { 
		pointWrap, pointClamp,
		linearWrap, linearClamp, 
		anisotropicWrap, anisotropicClamp };
}

float TreeBillboardsApp::GetHillsHeight(float x, float z)const
{
    return 0.3f*(z*sinf(0.1f*x) + x*cosf(0.1f*z));
}

XMFLOAT3 TreeBillboardsApp::GetHillsNormal(float x, float z)const
{
    // n = (-df/dx, 1, -df/dz)
    XMFLOAT3 n(
        -0.03f*z*cosf(0.1f*x) - 0.3f*cosf(0.1f*z),
        1.0f,
        -0.3f*sinf(0.1f*x) + 0.03f*x*sinf(0.1f*z));

    XMVECTOR unitNormal = XMVector3Normalize(XMLoadFloat3(&n));
    XMStoreFloat3(&n, unitNormal);

    return n;
}
