#include <Windows.h>
#include "Class.h"
#include "Offset.h"

auto X = 800, Y = 600, B = 32;
float SizeX, SizeY;
bool Debug, Misc;

int Draw(DWORD_PTR ULevel) {
	auto AController = Read<DWORD_PTR>(UPlayer + PlayerController);
	auto CameraManager = Read<DWORD_PTR>(AController + PlayerCameraManager);
	auto APawn = Read<DWORD_PTR>(AController + Pawn);
	auto ActorArray = Read<TArray>(ULevel + LevelArray);

	for (int i = 0; i < ActorArray.Count; ++i) {
		auto Actor = Read<DWORD_PTR>(ActorArray.Data + i * sizeof(DWORD_PTR));

		if (!Actor)
			continue;

		if (Actor == APawn)
			continue;

		auto ObjectIndex = Read<DWORD>(Actor + ComparisonIndex);
		auto ElementsPerChunk = 16 * 1024;
		auto NamesIndex = Read<DWORD_PTR>(GNames + ObjectIndex / ElementsPerChunk * sizeof(DWORD_PTR));
		auto NamesPointer = Read<DWORD_PTR>(NamesIndex + ObjectIndex % ElementsPerChunk * sizeof(DWORD_PTR));
		string ActorName = Read<FNameEntry>(NamesPointer).AnsiName;

		auto Name = ActorName;
		auto Color = Common;

		/* --------- Sample list --------- */

		// Debug
		if (Debug) {
		// Player
		} else if (ActorName.find("PlayerPawn") != -1) {
			auto PlayerIndex = Read<DWORD_PTR>(Actor + PlayerName);
			wstring PlayerString = Read<FString>(PlayerIndex).WideName;
			Name = string(PlayerString.begin(), PlayerString.end());
			Color = Player;
		// Cow
		} else if (ActorName.find("Cow_Character") != -1) {
			Name = "Cow";
			Color = Wildlife;
		// Giraffe
		} else if (ActorName.find("Giraffe_Character") != -1) {
			Name = "Giraffe";
			Color = Wildlife;
		// Monkey
		} else if (ActorName.find("Monkey_Character") != -1) {
			Name = "Monkey";
			Color = Wildlife;
		// Parrot
		} else if (ActorName.find("Parrot_Character") != -1) {
			Name = "Parrot";
			Color = Wildlife;
		} else {
			continue;
		}

		auto StatusComponent = Read<DWORD_PTR>(Actor + MyCharacterStatusComponent);
		auto CharacterLevel = Read<int>(StatusComponent + BaseCharacterLevel);
		Name += " (Lvl " + to_string(CharacterLevel) + ")";

		/* --------------------------------*/

		auto ActorRootComponent = Read<DWORD_PTR>(Actor + RootComponent);
		auto ActorComponent = Read<FTransform>(ActorRootComponent + ComponentToWorld);
		auto PlayerCamera = Read<FMinimalViewInfo, DWORD_PTR>(CameraManager + CameraCache + POV);

		auto Location = ActorComponent.Translation - PlayerCamera.Location;
		Name += " [" + to_string((int)sqrtf(Location.Dot(Location)) / 100) + "m]";
		wstring NameWide(Name.begin(), Name.end());

		auto Project = WorldToScreen(Location, PlayerCamera, SizeX, SizeY);
		RenderTarget->CreateSolidColorBrush(Color, &Brush);
		RenderTarget->SetTransform(Matrix3x2F::Translation(Project.X, Project.Y));
		RenderTarget->DrawText(NameWide.c_str(), (DWORD)NameWide.size(), TextFormat, RectF((float)X, (float)Y, 0, 0), Brush);
	}

	return 0;
}

int Render(HWND Window) {
	auto Target = FindWindow(0, GameName);
	if (Target) {
		DwmGetWindowAttribute(Target, DWMWA_EXTENDED_FRAME_BOUNDS, &Frame, sizeof(Frame));
		X = Frame.right - Frame.left;
		Y = Frame.bottom - Frame.top - B;
		MoveWindow(Window, Frame.left, Frame.top + B, X, Y, true);

		SizeX = (float)X / 2.f;
		SizeY = (float)Y / 2.f;
	}

	RenderTarget->Resize(SizeU(X, Y));
	RenderTarget->BeginDraw();
	RenderTarget->Clear();

	if (!UEngine) {
		Setup();
	} else if (Read<DWORD_PTR>(UEngine + ViewportClient)) {
		if (!UPlayer) {
			UViewport = Read<DWORD_PTR>(UEngine + ViewportClient);
			auto UGameInstance = Read<DWORD_PTR>(UViewport + GameInstance);
			auto ULocalPlayer = Read<DWORD_PTR>(UGameInstance + LocalPlayers);
			UPlayer = Read<DWORD_PTR>(ULocalPlayer);
		}

		if (Read<DWORD_PTR>(UViewport + World)) {
			auto UWorld = Read<DWORD_PTR>(UViewport + World);
			Draw(Read<DWORD_PTR>(UWorld + PersistentLevel));
		}
	}

	// Crosshair
	RenderTarget->CreateSolidColorBrush(Common, &Brush);
	RenderTarget->SetTransform(Matrix3x2F::Identity());
	RenderTarget->DrawLine(Point2F(SizeX + 10.f, SizeY), Point2F(SizeX - 10.f, SizeY), Brush, 1.f);
	RenderTarget->DrawLine(Point2F(SizeX, SizeY + 10.f), Point2F(SizeX, SizeY - 10.f), Brush, 1.f);

	RenderTarget->EndDraw();
	Sleep(5);

	// F5 Misc
	if (GetAsyncKeyState(VK_F5) & 1) {
		if (Misc)
			Misc = false;
		else
			Misc = true;
	// F8 Debug
	} else if (GetAsyncKeyState(VK_F8) & 1) {
		if (Debug)
			Debug = false;
		else
			Debug = true;
	// F12 Exit
	} else if (GetAsyncKeyState(VK_F12) & 1) {
		exit(1);
	}

	return 0;
}

LRESULT CALLBACK WinProc(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam) {
	switch (Message) {
		case WM_CREATE:
			D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &Factory);
			auto RenderProperties = RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT, PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED));
			Factory->CreateHwndRenderTarget(RenderProperties, HwndRenderTargetProperties(Window, SizeU(X, Y)), &RenderTarget);
			DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown**)&DWriteFactory);
			DWriteFactory->CreateTextFormat(L"Tahoma", 0, DWRITE_FONT_WEIGHT_THIN, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 12, L"en-us", &TextFormat);
			return 0;

		case WM_PAINT:
			Render(Window);
			return 0;

		case WM_DESTROY:
			exit(1);
	}

	return DefWindowProc(Window, Message, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE Instance, HINSTANCE, LPSTR, INT) {
	MSG Message;
	MARGINS Margin = {-1};
	WNDCLASS wclass = {CS_HREDRAW | CS_VREDRAW, WinProc, 0, 0, Instance, 0, LoadCursor(0, IDC_ARROW), 0, 0, ItemName};
	RegisterClass(&wclass);

	auto Overlay = CreateWindowEx(WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_TOPMOST, ItemName, ItemName, WS_POPUP | WS_VISIBLE, 0, 0, X, Y, 0, 0, 0, 0);
	SetLayeredWindowAttributes(Overlay, 0, 255, LWA_ALPHA);
	DwmExtendFrameIntoClientArea(Overlay, &Margin);

	while (GetMessage(&Message, Overlay, 0, 0)) {
		TranslateMessage(&Message);
		DispatchMessage(&Message);
	}

	return 0;
}