#include "EscapeRoom/EscapeHUD.h"

#include "Engine/GameViewportClient.h"
#include "EngineUtils.h"
#include "EscapeRoom/EscapeCharacter.h"
#include "EscapeRoom/EscapeRoomManager.h"
#include "GameFramework/PlayerController.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "EscapeRoom"

void AEscapeHUD::BeginPlay()
{
	Super::BeginPlay();

	UGameViewportClient* Viewport = GetWorld()->GetGameViewport();
	if (!Viewport)
	{
		return;
	}

	const TWeakObjectPtr<AEscapeHUD> Self(this);
	const FSlateFontInfo Large = FCoreStyle::GetDefaultFontStyle("Bold", 22);
	const FSlateFontInfo Medium = FCoreStyle::GetDefaultFontStyle("Regular", 17);
	const FSlateFontInfo Small = FCoreStyle::GetDefaultFontStyle("Regular", 13);
	const FLinearColor Shadow(0.f, 0.f, 0.f, 0.85f);

	auto ManagerText = [Self](FText (AEscapeRoomManager::*Getter)() const)
	{
		return [Self, Getter]()
		{
			const AEscapeRoomManager* Manager = Self.IsValid() ? Self->GetManager() : nullptr;
			return Manager ? (Manager->*Getter)() : FText::GetEmpty();
		};
	};

	// Lines with nothing to show collapse, so the backing panel only covers real text.
	auto ManagerLineVisibility = [Self](FText (AEscapeRoomManager::*Getter)() const)
	{
		return [Self, Getter]()
		{
			const AEscapeRoomManager* Manager = Self.IsValid() ? Self->GetManager() : nullptr;
			return Manager && !(Manager->*Getter)().IsEmpty() ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
		};
	};

	Overlay = SNew(SOverlay)
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
		[
			SNew(STextBlock).Text(FText::FromString(TEXT("+"))).Font(Medium)
			.ColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 0.75f)).ShadowOffset(FVector2D(1.f, 1.f)).ShadowColorAndOpacity(Shadow)
		]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(FMargin(0.f, 80.f, 0.f, 0.f))
		[
			SNew(STextBlock).Text_Lambda([Self]() { return Self.IsValid() ? Self->GetPrompt() : FText::GetEmpty(); })
			.Font(Medium).ColorAndOpacity(FLinearColor(1.f, 0.92f, 0.6f)).ShadowOffset(FVector2D(1.f, 1.f)).ShadowColorAndOpacity(Shadow)
		]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(FMargin(40.f, 0.f, 40.f, 110.f))
		[
			SNew(STextBlock).Text_Lambda([Self]() { return Self.IsValid() ? Self->GetMessage() : FText::GetEmpty(); })
			.Font(Large).Justification(ETextJustify::Center).WrapTextAt(1000.f)
			.ShadowOffset(FVector2D(2.f, 2.f)).ShadowColorAndOpacity(Shadow)
		]
		+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(FMargin(20.f, 16.f))
		[
			SNew(SBorder).BorderImage(&PanelBrush).Padding(FMargin(14.f, 8.f, 16.f, 10.f))
			[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock).Text_Lambda(ManagerText(&AEscapeRoomManager::GetStatusText))
				.Font(Medium).ShadowOffset(FVector2D(1.f, 1.f)).ShadowColorAndOpacity(Shadow)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.f, 6.f, 0.f, 0.f))
			[
				SNew(STextBlock).Text_Lambda(ManagerText(&AEscapeRoomManager::GetInventoryText))
				.Visibility_Lambda(ManagerLineVisibility(&AEscapeRoomManager::GetInventoryText))
				.Font(Small).ColorAndOpacity(FLinearColor(0.85f, 0.85f, 0.85f)).ShadowOffset(FVector2D(1.f, 1.f)).ShadowColorAndOpacity(Shadow)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.f, 4.f, 0.f, 0.f))
			[
				SNew(STextBlock).Text_Lambda(ManagerText(&AEscapeRoomManager::GetPaletteText))
				.Visibility_Lambda(ManagerLineVisibility(&AEscapeRoomManager::GetPaletteText))
				.Font(Small).ColorAndOpacity(FLinearColor(0.7f, 1.f, 0.75f)).ShadowOffset(FVector2D(1.f, 1.f)).ShadowColorAndOpacity(Shadow)
			]
			]
		]
		+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(FMargin(28.f, 22.f))
		[
			SNew(STextBlock).Text(LOCTEXT("Controls", "WASD 이동   마우스 시점   E 상호작용"))
			.Font(Small).ColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 0.55f)).ShadowOffset(FVector2D(1.f, 1.f)).ShadowColorAndOpacity(Shadow)
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
	if (!Player || (Manager && Manager->HasEscaped()))
	{
		return FText::GetEmpty();
	}
	return Player->GetFocusPrompt();
}

FText AEscapeHUD::GetMessage() const
{
	const AEscapeRoomManager* Manager = GetManager();
	return Manager && Manager->IsMessageVisible() ? Manager->GetMessage() : FText::GetEmpty();
}

#undef LOCTEXT_NAMESPACE
