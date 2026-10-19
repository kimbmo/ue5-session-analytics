#include "EscapeRoom/EscapeHUD.h"

#include "Engine/GameViewportClient.h"
#include "EngineUtils.h"
#include "EscapeRoom/EscapeCharacter.h"
#include "EscapeRoom/EscapeRoomManager.h"
#include "GameFramework/PlayerController.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SCanvas.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "EscapeRoom"

namespace
{
	const FLinearColor Shadow(0.f, 0.f, 0.f, 0.85f);
	const FLinearColor KeyGold(1.f, 0.72f, 0.15f);
	const FLinearColor DoorKeyColor(0.85f, 0.85f, 0.9f);
	const FLinearColor PaletteWood(0.8f, 0.62f, 0.38f);
	const FLinearColor LabelColor(0.88f, 0.88f, 0.88f);

	/** One canvas element: position and size in icon pixels. */
	struct FIconPart { FVector2D Position; FVector2D Size; const FSlateBrush* Brush; FLinearColor Color; };

	TSharedRef<SWidget> MakeIconCanvas(const FVector2D& Size, const TArray<FIconPart>& Parts)
	{
		TSharedRef<SCanvas> Canvas = SNew(SCanvas);
		for (const FIconPart& Part : Parts)
		{
			Canvas->AddSlot().Position(Part.Position).Size(Part.Size)
			[
				SNew(SImage).Image(Part.Brush).ColorAndOpacity(Part.Color)
			];
		}
		return SNew(SBox).WidthOverride(Size.X).HeightOverride(Size.Y)[Canvas];
	}
}

const FSlateBrush* AEscapeHUD::MakeBrush(const FLinearColor& Fill, float Radius, const FLinearColor& Outline, float OutlineWidth)
{
	return &Brushes.Add_GetRef(MakeShared<FSlateRoundedBoxBrush>(Fill, Radius, Outline, OutlineWidth)).Get();
}

TSharedRef<SWidget> AEscapeHUD::MakeKeyIcon(const FLinearColor& Color)
{
	const FSlateBrush* Ring = MakeBrush(FLinearColor::Transparent, 10.f, FLinearColor::White, 4.f);
	const FSlateBrush* Solid = MakeBrush(FLinearColor::White, 1.5f);
	return MakeIconCanvas(FVector2D(56.f, 40.f), {
		{ FVector2D(2.f, 10.f), FVector2D(20.f, 20.f), Ring, Color },
		{ FVector2D(20.f, 17.f), FVector2D(32.f, 6.f), Solid, Color },
		{ FVector2D(40.f, 23.f), FVector2D(4.f, 8.f), Solid, Color },
		{ FVector2D(47.f, 23.f), FVector2D(4.f, 6.f), Solid, Color },
	});
}

TSharedRef<SWidget> AEscapeHUD::MakePaletteIcon()
{
	const FSlateBrush* Board = MakeBrush(FLinearColor::White, 17.f);
	const FSlateBrush* Dot = MakeBrush(FLinearColor::White, 4.f);
	return MakeIconCanvas(FVector2D(56.f, 40.f), {
		{ FVector2D(2.f, 3.f), FVector2D(52.f, 34.f), Board, PaletteWood },
		{ FVector2D(38.f, 21.f), FVector2D(8.f, 8.f), Dot, FLinearColor(0.08f, 0.05f, 0.03f) },
		{ FVector2D(7.f, 17.f), FVector2D(8.f, 8.f), Dot, FLinearColor(0.8f, 0.08f, 0.06f) },
		{ FVector2D(11.f, 8.f), FVector2D(8.f, 8.f), Dot, FLinearColor(0.1f, 0.25f, 0.9f) },
		{ FVector2D(21.f, 6.f), FVector2D(8.f, 8.f), Dot, FLinearColor(0.95f, 0.95f, 0.93f) },
		{ FVector2D(31.f, 8.f), FVector2D(8.f, 8.f), Dot, FLinearColor(1.f, 0.8f, 0.1f) },
		{ FVector2D(15.f, 25.f), FVector2D(8.f, 8.f), Dot, FLinearColor(0.02f, 0.02f, 0.02f) },
	});
}

TSharedRef<SWidget> AEscapeHUD::MakePaintSlotIcon(int32 Slot)
{
	const FSlateBrush* Frame = MakeBrush(FLinearColor::Transparent, 6.f, FLinearColor::White, 2.f);
	const FSlateBrush* Fill = MakeBrush(FLinearColor::White, 4.f);
	const TWeakObjectPtr<AEscapeHUD> Self(this);
	return SNew(SBox).WidthOverride(40.f).HeightOverride(40.f)
	[
		SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SImage).Image(Frame).ColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 0.7f))
		]
		+ SOverlay::Slot().Padding(5.f)
		[
			SNew(SImage).Image(Fill)
			.ColorAndOpacity_Lambda([Self, Slot]()
			{
				FLinearColor Color;
				const AEscapeRoomManager* Manager = Self.IsValid() ? Self->GetManager() : nullptr;
				return FSlateColor(Manager && Manager->GetPaintSlotColor(Slot, Color) ? Color : FLinearColor::Transparent);
			})
		]
	];
}

TSharedRef<SWidget> AEscapeHUD::MakeItem(TSharedRef<SWidget> Icon, const FText& Label, TFunction<bool()> IsVisible, FName Item)
{
	const TWeakObjectPtr<AEscapeHUD> Self(this);
	const FSlateBrush* FlashRing = MakeBrush(FLinearColor::Transparent, 6.f, FLinearColor::White, 3.f);
	return SNew(SOverlay)
		.Visibility_Lambda([IsVisible]() { return IsVisible() ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
		+ SOverlay::Slot()
		[
			SNew(SBorder).BorderImage(&PanelBrush).Padding(FMargin(10.f, 8.f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
				[
					Icon
				]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(FMargin(0.f, 4.f, 0.f, 0.f))
				[
					SNew(STextBlock).Text(Label).Font(FCoreStyle::GetDefaultFontStyle("Regular", 12)).ColorAndOpacity(LabelColor)
				]
			]
		]
		// v5: yellow ring that blinks right after the item was picked up.
		+ SOverlay::Slot()
		[
			SNew(SImage).Image(FlashRing)
			.ColorAndOpacity_Lambda([Self, Item]()
			{
				const AEscapeRoomManager* Manager = Self.IsValid() ? Self->GetManager() : nullptr;
				const float Flash = Manager && !Item.IsNone() ? Manager->GetItemFlash(Item) : 0.f;
				return FSlateColor(FLinearColor(1.f, 0.85f, 0.2f, Flash));
			})
		];
}

TSharedRef<SWidget> AEscapeHUD::MakeMenu()
{
	const TWeakObjectPtr<AEscapeHUD> Self(this);
	auto Manager = [Self]() -> AEscapeRoomManager* { return Self.IsValid() ? Self->GetManager() : nullptr; };
	const FSlateFontInfo Title = FCoreStyle::GetDefaultFontStyle("Bold", 26);
	const FSlateFontInfo Button = FCoreStyle::GetDefaultFontStyle("Bold", 18);
	const FSlateFontInfo Body = FCoreStyle::GetDefaultFontStyle("Regular", 16);
	const FSlateFontInfo Small = FCoreStyle::GetDefaultFontStyle("Regular", 13);

	auto MenuButton = [Button](const FText& Label, TFunction<void()> OnClick)
	{
		return SNew(SButton).HAlign(HAlign_Center).ContentPadding(FMargin(16.f, 8.f))
			.OnClicked_Lambda([OnClick]() { OnClick(); return FReply::Handled(); })
			[
				SNew(STextBlock).Text(Label).Font(Button)
			];
	};

	auto MainVisibility = [Manager]() { const AEscapeRoomManager* M = Manager(); return M && !M->IsQuitConfirmOpen() ? EVisibility::Visible : EVisibility::Collapsed; };
	auto ConfirmVisibility = [Manager]() { const AEscapeRoomManager* M = Manager(); return M && M->IsQuitConfirmOpen() ? EVisibility::Visible : EVisibility::Collapsed; };

	FNumberFormattingOptions TwoDecimals;
	TwoDecimals.SetMinimumFractionalDigits(2).SetMaximumFractionalDigits(2);

	return SNew(SBorder).BorderImage(MakeBrush(FLinearColor(0.f, 0.f, 0.f, 0.6f), 0.f))
		.HAlign(HAlign_Center).VAlign(VAlign_Center)
		.Visibility_Lambda([Manager]() { const AEscapeRoomManager* M = Manager(); return M && M->IsMenuOpen() ? EVisibility::Visible : EVisibility::Collapsed; })
		[
			SNew(SBorder).BorderImage(MakeBrush(FLinearColor(0.06f, 0.06f, 0.07f, 0.96f), 10.f)).Padding(FMargin(32.f, 26.f))
			[
				SNew(SBox).WidthOverride(420.f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(FMargin(0.f, 0.f, 0.f, 20.f))
					[
						SNew(STextBlock).Text(LOCTEXT("MenuTitle", "일시 정지")).Font(Title)
					]
					// Main page
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SVerticalBox).Visibility_Lambda(MainVisibility)
						+ SVerticalBox::Slot().AutoHeight()
						[
							MenuButton(LOCTEXT("MenuContinue", "계속하기"), [Manager]() { if (AEscapeRoomManager* M = Manager()) M->CloseMenu(); })
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.f, 22.f, 0.f, 4.f))
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().FillWidth(1.f)
							[
								SNew(STextBlock).Text(LOCTEXT("MenuSensitivity", "마우스 감도")).Font(Body)
							]
							+ SHorizontalBox::Slot().AutoWidth()
							[
								SNew(STextBlock).Font(Body).ColorAndOpacity(FLinearColor(1.f, 0.88f, 0.35f))
								.Text_Lambda([Manager, TwoDecimals]() { const AEscapeRoomManager* M = Manager(); return FText::AsNumber(M ? M->GetMouseSensitivity() : 0.f, &TwoDecimals); })
							]
						]
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(SSlider)
							.MinValue(Manager() ? Manager()->MinMouseSensitivity : 0.1f)
							.MaxValue(Manager() ? Manager()->MaxMouseSensitivity : 1.5f)
							.StepSize(0.05f)
							.Value_Lambda([Manager]() { const AEscapeRoomManager* M = Manager(); return M ? M->GetMouseSensitivity() : 0.5f; })
							.OnValueChanged_Lambda([Manager](float Value) { if (AEscapeRoomManager* M = Manager()) M->SetMouseSensitivity(Value); })
							.OnMouseCaptureEnd_Lambda([Manager]() { if (AEscapeRoomManager* M = Manager()) M->CommitMouseSensitivity(); })
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.f, 22.f, 0.f, 0.f))
						[
							MenuButton(LOCTEXT("MenuQuit", "게임 종료"), [Manager]() { if (AEscapeRoomManager* M = Manager()) M->RequestQuit(); })
						]
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(FMargin(0.f, 18.f, 0.f, 0.f))
						[
							SNew(STextBlock).Text(LOCTEXT("MenuKeys", "Esc 계속하기 · Q 게임 종료")).Font(Small).ColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 0.6f))
						]
					]
					// Quit confirmation
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SVerticalBox).Visibility_Lambda(ConfirmVisibility)
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
						[
							SNew(STextBlock).Text(LOCTEXT("QuitConfirm", "정말 종료할까요?")).Font(Button)
						]
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(FMargin(0.f, 6.f, 0.f, 18.f))
						[
							SNew(STextBlock).Text(LOCTEXT("QuitConfirmNote", "지금까지의 기록은 저장돼요.")).Font(Small).ColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 0.6f))
						]
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().FillWidth(1.f).Padding(FMargin(0.f, 0.f, 8.f, 0.f))
							[
								MenuButton(LOCTEXT("QuitYes", "예 (Y)"), [Manager]() { if (AEscapeRoomManager* M = Manager()) M->ConfirmQuit(); })
							]
							+ SHorizontalBox::Slot().FillWidth(1.f).Padding(FMargin(8.f, 0.f, 0.f, 0.f))
							[
								MenuButton(LOCTEXT("QuitNo", "아니오 (N)"), [Manager]() { if (AEscapeRoomManager* M = Manager()) M->CancelQuit(); })
							]
						]
					]
				]
			]
		];
}

void AEscapeHUD::BeginPlay()
{
	Super::BeginPlay();

	UGameViewportClient* Viewport = GetWorld()->GetGameViewport();
	if (!Viewport)
	{
		return;
	}

	const TWeakObjectPtr<AEscapeHUD> Self(this);
	const FSlateFontInfo Large = FCoreStyle::GetDefaultFontStyle("Bold", 26);
	const FSlateFontInfo HintFont = FCoreStyle::GetDefaultFontStyle("Bold", 28);
	const FSlateFontInfo Crosshair = FCoreStyle::GetDefaultFontStyle("Bold", 20);
	const FSlateFontInfo Medium = FCoreStyle::GetDefaultFontStyle("Regular", 17);
	const FSlateFontInfo Small = FCoreStyle::GetDefaultFontStyle("Regular", 13);

	auto ManagerText = [Self](FText (AEscapeRoomManager::*Getter)() const)
	{
		return [Self, Getter]()
		{
			const AEscapeRoomManager* Manager = Self.IsValid() ? Self->GetManager() : nullptr;
			return Manager ? (Manager->*Getter)() : FText::GetEmpty();
		};
	};
	auto ManagerFlag = [Self](bool (AEscapeRoomManager::*Getter)() const)
	{
		return [Self, Getter]()
		{
			const AEscapeRoomManager* Manager = Self.IsValid() ? Self->GetManager() : nullptr;
			return Manager && (Manager->*Getter)();
		};
	};

	Overlay = SNew(SOverlay).Visibility(EVisibility::SelfHitTestInvisible)
		// Crosshair: yellow while something interactable is under it; pops on E (yellow = used, red = nothing there).
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
		[
			SNew(STextBlock).Text(FText::FromString(TEXT("+"))).Font(Crosshair)
			.ShadowOffset(FVector2D(1.f, 1.f)).ShadowColorAndOpacity(Shadow)
			.RenderTransformPivot(FVector2D(0.5f, 0.5f))
			.ColorAndOpacity_Lambda([Self]()
			{
				const AEscapeCharacter* Player = Self.IsValid() ? Self->GetPlayerCharacter() : nullptr;
				if (Player && Player->GetSecondsSinceInteract() < 0.3f)
				{
					return FSlateColor(Player->LastInteractHit() ? FLinearColor(1.f, 0.85f, 0.2f) : FLinearColor(1.f, 0.35f, 0.3f));
				}
				return FSlateColor(Player && Player->HasFocus() ? FLinearColor(1.f, 0.9f, 0.4f) : FLinearColor(1.f, 1.f, 1.f, 0.75f));
			})
			.RenderTransform_Lambda([Self]()
			{
				const AEscapeCharacter* Player = Self.IsValid() ? Self->GetPlayerCharacter() : nullptr;
				const float Since = Player ? Player->GetSecondsSinceInteract() : 1.f;
				const float Pop = Since < 0.3f ? 1.f + 0.8f * (1.f - Since / 0.3f) : (Player && Player->HasFocus() ? 1.25f : 1.f);
				return TOptional<FSlateRenderTransform>(FSlateRenderTransform(Pop));
			})
		]
		// Hint line: large and top-centre, so it cannot be missed but never covers the object the hint points at.
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Top).Padding(FMargin(0.f, 70.f, 0.f, 0.f))
		[
			SNew(SBorder).BorderImage(&PanelBrush).Padding(FMargin(22.f, 10.f))
			.Visibility_Lambda([Self]() { return Self.IsValid() && !Self->GetHintMessage().IsEmpty() ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
			[
				SNew(STextBlock).Text_Lambda([Self]() { return Self.IsValid() ? Self->GetHintMessage() : FText::GetEmpty(); })
				.Font(HintFont).ColorAndOpacity(FLinearColor(1.f, 0.88f, 0.35f)).Justification(ETextJustify::Center)
			]
		]
		// v5: big pickup line above the crosshair ("서랍 열쇠 획득!"). The bottom message alone was missed by a v4 tester.
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(FMargin(0.f, 0.f, 0.f, 240.f))
		[
			SNew(SBorder).BorderImage(&PanelBrush).Padding(FMargin(28.f, 12.f))
			.Visibility_Lambda([Self]()
			{
				const AEscapeRoomManager* Manager = Self.IsValid() ? Self->GetManager() : nullptr;
				return Manager && !Manager->GetItemToast().IsEmpty() && !Manager->IsMenuOpen() ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
			})
			[
				SNew(STextBlock).Text_Lambda(ManagerText(&AEscapeRoomManager::GetItemToast))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 36)).ColorAndOpacity(FLinearColor(1.f, 0.85f, 0.2f))
				.ShadowOffset(FVector2D(2.f, 2.f)).ShadowColorAndOpacity(Shadow)
			]
		]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(FMargin(0.f, 80.f, 0.f, 0.f))
		[
			SNew(STextBlock).Text_Lambda([Self]() { return Self.IsValid() ? Self->GetPrompt() : FText::GetEmpty(); })
			.Font(Medium).ColorAndOpacity(FLinearColor(1.f, 0.92f, 0.6f)).ShadowOffset(FVector2D(1.f, 1.f)).ShadowColorAndOpacity(Shadow)
		]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(FMargin(40.f, 0.f, 40.f, 100.f))
		[
			SNew(SBorder).BorderImage(&PanelBrush).Padding(FMargin(22.f, 10.f))
			.Visibility_Lambda([Self]() { return Self.IsValid() && !Self->GetMessage().IsEmpty() ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
			[
				SNew(STextBlock).Text_Lambda([Self]() { return Self.IsValid() ? Self->GetMessage() : FText::GetEmpty(); })
				.Font(Large).Justification(ETextJustify::Center).WrapTextAt(1100.f)
				.ShadowOffset(FVector2D(2.f, 2.f)).ShadowColorAndOpacity(Shadow)
			]
		]
		// Puzzle status, top left.
		+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(FMargin(20.f, 16.f))
		[
			SNew(SBorder).BorderImage(&PanelBrush).Padding(FMargin(14.f, 8.f, 16.f, 10.f))
			[
				SNew(STextBlock).Text_Lambda(ManagerText(&AEscapeRoomManager::GetStatusText))
				.Font(Medium).ShadowOffset(FVector2D(1.f, 1.f)).ShadowColorAndOpacity(Shadow)
			]
		]
		// Hint counter, top right: always visible while a puzzle runs, so players know H exists.
		+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(FMargin(20.f, 16.f))
		[
			SNew(SBorder).BorderImage(&PanelBrush).Padding(FMargin(14.f, 8.f, 16.f, 10.f))
			.Visibility_Lambda([Self]()
			{
				const AEscapeRoomManager* Manager = Self.IsValid() ? Self->GetManager() : nullptr;
				return Manager && !Manager->GetHintCounterText().IsEmpty() ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
			})
			[
				SNew(STextBlock).Text_Lambda(ManagerText(&AEscapeRoomManager::GetHintCounterText))
				.Font(Medium).ColorAndOpacity(FLinearColor(1.f, 0.88f, 0.35f)).ShadowOffset(FVector2D(1.f, 1.f)).ShadowColorAndOpacity(Shadow)
			]
		]
		// Held items, bottom left: drawer key, palette, the two chosen paints, door key.
		+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Bottom).Padding(FMargin(20.f, 0.f, 0.f, 20.f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin(0.f, 0.f, 10.f, 0.f))
			[
				MakeItem(MakeKeyIcon(KeyGold), LOCTEXT("ItemDrawerKey", "서랍 열쇠"), ManagerFlag(&AEscapeRoomManager::HasDrawerKeyItem), TEXT("drawer_key"))
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin(0.f, 0.f, 10.f, 0.f))
			[
				MakeItem(MakePaletteIcon(), LOCTEXT("ItemPalette", "팔레트"), ManagerFlag(&AEscapeRoomManager::HasPaletteItem), TEXT("palette"))
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin(0.f, 0.f, 10.f, 0.f))
			[
				MakeItem(MakePaintSlotIcon(0), LOCTEXT("ItemPaint1", "물감 1"), ManagerFlag(&AEscapeRoomManager::ShowsPaintSlots))
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin(0.f, 0.f, 10.f, 0.f))
			[
				MakeItem(MakePaintSlotIcon(1), LOCTEXT("ItemPaint2", "물감 2"), ManagerFlag(&AEscapeRoomManager::ShowsPaintSlots))
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				MakeItem(MakeKeyIcon(DoorKeyColor), LOCTEXT("ItemDoorKey", "문 열쇠"), ManagerFlag(&AEscapeRoomManager::HasDoorKeyItem), TEXT("door_key"))
			]
		]
		+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(FMargin(28.f, 22.f))
		[
			SNew(STextBlock).Text(LOCTEXT("Controls", "WASD 이동   마우스 시점   E 상호작용   H 힌트   Esc 메뉴"))
			.Font(Small).ColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 0.55f)).ShadowOffset(FVector2D(1.f, 1.f)).ShadowColorAndOpacity(Shadow)
		]
		// Esc menu on top of everything.
		+ SOverlay::Slot().HAlign(HAlign_Fill).VAlign(VAlign_Fill)
		[
			MakeMenu()
		];

	Viewport->AddViewportWidgetContent(Overlay.ToSharedRef(), 10);
}

void AEscapeHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Overlay.IsValid())
	{
		if (UGameViewportClient* Viewport = GetWorld()->GetGameViewport())
		{
			Viewport->RemoveViewportWidgetContent(Overlay.ToSharedRef());
		}
		Overlay.Reset();
	}
	Super::EndPlay(EndPlayReason);
}

AEscapeRoomManager* AEscapeHUD::GetManager() const
{
	if (!CachedManager.IsValid())
	{
		for (TActorIterator<AEscapeRoomManager> It(GetWorld()); It; ++It)
		{
			CachedManager = *It;
			break;
		}
	}
	return CachedManager.Get();
}

FText AEscapeHUD::GetPrompt() const
{
	const AEscapeCharacter* Player = PlayerOwner ? Cast<AEscapeCharacter>(PlayerOwner->GetPawn()) : nullptr;
	const AEscapeRoomManager* Manager = GetManager();
	if (!Player || (Manager && (Manager->HasEscaped() || Manager->IsMenuOpen())))
	{
		return FText::GetEmpty();
	}
	return Player->GetFocusPrompt();
}

const AEscapeCharacter* AEscapeHUD::GetPlayerCharacter() const
{
	return PlayerOwner ? Cast<AEscapeCharacter>(PlayerOwner->GetPawn()) : nullptr;
}

FText AEscapeHUD::GetHintMessage() const
{
	const AEscapeRoomManager* Manager = GetManager();
	return Manager && Manager->IsHintMessageVisible() ? Manager->GetHintMessage() : FText::GetEmpty();
}

FText AEscapeHUD::GetMessage() const
{
	const AEscapeRoomManager* Manager = GetManager();
	return Manager && Manager->IsMessageVisible() ? Manager->GetMessage() : FText::GetEmpty();
}

#undef LOCTEXT_NAMESPACE
