// Development-only console command that plays the room start to finish through the real E trace,
// takes screenshots along the way and (optionally) quits. Used to verify the analytics CSV end to end.
//
//   Escape.Autoplay [HintDelaySeconds=2] [quit]
//
// HintDelay1/2 are shortened to HintDelaySeconds / 2x so every hint level fires during the run.

#if !UE_BUILD_SHIPPING

#include "Camera/CameraComponent.h"
#include "Containers/Ticker.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "EscapeRoom/EscapeCharacter.h"
#include "EscapeRoom/EscapeDesk.h"
#include "EscapeRoom/EscapeDoor.h"
#include "EscapeRoom/EscapeFrame.h"
#include "EscapeRoom/EscapePaint.h"
#include "EscapeRoom/EscapeRoomManager.h"
#include "EscapeRoom/EscapeSafe.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Misc/CoreDelegates.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

DEFINE_LOG_CATEGORY_STATIC(LogEscapeAutoplay, Log, All);

namespace
{
	constexpr float StandDistance = 110.f;
	constexpr double WaitTimeout = 120.0;

	class FEscapeAutoplay
	{
	public:
		FEscapeAutoplay(UWorld* InWorld, float HintDelay, bool bInQuit, bool bCeilingOnly)
			: World(InWorld), bQuit(bInQuit)
		{
			if (AEscapeRoomManager* Manager = GetManager())
			{
				Manager->HintDelay1 = HintDelay;
				Manager->HintDelay2 = HintDelay * 2.f;
				Manager->NotifyProgress(Manager->GetCurrentPuzzle());
			}
			if (bCeilingOnly)
			{
				BuildCeilingScript();
			}
			else
			{
				BuildScript();
			}
			TickerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateRaw(this, &FEscapeAutoplay::Tick));
		}

		~FEscapeAutoplay()
		{
			FTSTicker::GetCoreTicker().RemoveTicker(TickerHandle);
		}

	private:
		struct FStep
		{
			FString Label;
			float Delay = 0.5f;
			TFunction<void()> Action;
			TFunction<bool()> WaitUntil;
		};

		void Add(const FString& Label, float Delay, TFunction<void()> Action, TFunction<bool()> WaitUntil = nullptr)
		{
			Steps.Add({ Label, Delay, MoveTemp(Action), MoveTemp(WaitUntil) });
		}

		void WaitHint(const FString& Label, int32 Level)
		{
			Add(Label, 0.f, [] {}, [this, Level] { const AEscapeRoomManager* M = GetManager(); return M && M->GetHintLevel() >= Level; });
		}

		bool Tick(float DeltaTime)
		{
			if (!World.IsValid() || Index >= Steps.Num())
			{
				Finish();
				return false;
			}
			const double Now = FPlatformTime::Seconds();
			FStep& Step = Steps[Index];
			if (!bStarted)
			{
				if (Now < StepTime + Step.Delay)
				{
					return true;
				}
				UE_LOG(LogEscapeAutoplay, Display, TEXT("Autoplay %d/%d: %s"), Index + 1, Steps.Num(), *Step.Label);
				Step.Action();
				bStarted = true;
				StepTime = Now;
			}
			if (Step.WaitUntil && !Step.WaitUntil())
			{
				if (Now - StepTime < WaitTimeout)
				{
					return true;
				}
				UE_LOG(LogEscapeAutoplay, Error, TEXT("Autoplay: timed out waiting on '%s'"), *Step.Label);
			}
			++Index;
			bStarted = false;
			StepTime = Now;
			return true;
		}

		void Finish();

		AEscapeRoomManager* GetManager() const
		{
			TActorIterator<AEscapeRoomManager> It(World.Get());
			return It ? *It : nullptr;
		}

		template <typename T>
		T* Find(const FString& ObjectId = FString()) const
		{
			for (TActorIterator<T> It(World.Get()); It; ++It)
			{
				if (ObjectId.IsEmpty() || It->ObjectId == ObjectId)
				{
					return *It;
				}
			}
			UE_LOG(LogEscapeAutoplay, Error, TEXT("Autoplay: no %s with id '%s'"), *T::StaticClass()->GetName(), *ObjectId);
			return nullptr;
		}

		AEscapeCharacter* GetPlayer() const
		{
			const APlayerController* PC = World.IsValid() ? World->GetFirstPlayerController() : nullptr;
			return PC ? Cast<AEscapeCharacter>(PC->GetPawn()) : nullptr;
		}

		void StandAndLook(const FVector& Stand, const FVector& Target) const
		{
			AEscapeCharacter* Player = GetPlayer();
			if (!Player)
			{
				return;
			}
			Player->SetActorLocation(FVector(Stand.X, Stand.Y, Player->GetActorLocation().Z), false, nullptr, ETeleportType::TeleportPhysics);
			const FRotator View = (Target - Player->Camera->GetComponentLocation()).Rotation();
			Player->GetController()->SetControlRotation(View);
		}

		/**
		 * Stands in front of the component (along its actor's forward) and aims at the middle of its front face,
		 * or at the middle of its top for flat things lying on a surface (bTop).
		 */
		void LookAt(const UPrimitiveComponent* Component, bool bTop = false) const
		{
			if (!Component)
			{
				return;
			}
			const FVector Forward = Component->GetOwner()->GetActorForwardVector().GetSafeNormal2D();
			const FBoxSphereBounds& Bounds = Component->Bounds;
			const float FrontDepth = FMath::Abs(Forward.X) * Bounds.BoxExtent.X + FMath::Abs(Forward.Y) * Bounds.BoxExtent.Y;
			const FVector Aim = bTop ? Bounds.Origin + FVector(0.f, 0.f, Bounds.BoxExtent.Z) : Bounds.Origin + Forward * FrontDepth * 0.8f;
			StandAndLook(Aim + Forward * StandDistance, Aim);
		}

		void Press(const UPrimitiveComponent* Component, int32 Times = 1, bool bTop = false) const
		{
			LookAt(Component, bTop);
			AEscapeCharacter* Player = GetPlayer();
			for (int32 Count = 0; Player && Count < Times; ++Count)
			{
				if (!Player->Interact())
				{
					UE_LOG(LogEscapeAutoplay, Error, TEXT("Autoplay: trace missed %s"), *GetNameSafe(Component));
				}
			}
		}

		void Shot(const FString& Name) const
		{
			const FString Path = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Screenshots/EscapeRoom") / (Name + TEXT(".png")));
			FScreenshotRequest::RequestScreenshot(Path, true, false);
			UE_LOG(LogEscapeAutoplay, Display, TEXT("Autoplay: screenshot %s"), *Path);
		}

		/** From PlayerStart at eye height, looks up ~60 degrees in four directions (checks what is visible on the ceiling). */
		void BuildCeilingScript()
		{
			for (int32 Yaw = 0; Yaw < 360; Yaw += 90)
			{
				Add(FString::Printf(TEXT("look up yaw %d"), Yaw), 1.0f, [this, Yaw]
				{
					StandAndLook(FVector(-220.f, 0.f, 0.f), FVector(-220.f, 0.f, 0.f));
					if (AEscapeCharacter* Player = GetPlayer())
					{
						Player->GetController()->SetControlRotation(FRotator(60.f, Yaw, 0.f));
					}
				});
				Add(TEXT("shot"), 0.6f, [this, Yaw] { Shot(FString::Printf(TEXT("ceiling_yaw%03d"), Yaw)); });
			}
			// Under the lamp, looking up at the wall/ceiling corner above the door.
			const FVector Spots[] = { FVector(200.f, 0.f, 0.f), FVector(230.f, -60.f, 0.f), FVector(150.f, 40.f, 0.f) };
			for (int32 SpotIndex = 0; SpotIndex < UE_ARRAY_COUNT(Spots); ++SpotIndex)
			{
				const FVector Spot = Spots[SpotIndex];
				Add(TEXT("look up near door"), 0.6f, [this, Spot] { StandAndLook(Spot, FVector(310.f, Spot.Y * 0.3f, 300.f)); });
				Add(TEXT("shot"), 0.6f, [this, SpotIndex] { Shot(FString::Printf(TEXT("ceiling_door%d"), SpotIndex)); });
			}
		}

		void BuildScript()
		{
			const FVector FrameWall(60.f, 297.f, 165.f);

			Add(TEXT("overview"), 1.5f, [this] { StandAndLook(FVector(-270.f, -40.f, 0.f), FVector(100.f, 200.f, 120.f)); });
			Add(TEXT("shot overview"), 0.6f, [this] { Shot(TEXT("00_overview_frames")); });
			Add(TEXT("overview 2"), 0.4f, [this] { StandAndLook(FVector(240.f, 60.f, 0.f), FVector(-180.f, -100.f, 90.f)); });
			Add(TEXT("shot overview 2"), 0.6f, [this] { Shot(TEXT("00_overview_desk_easel_safe")); });
			Add(TEXT("spawn view"), 0.4f, [this] { StandAndLook(FVector(-220.f, 0.f, 0.f), FVector(300.f, 0.f, 150.f)); });
			Add(TEXT("shot start"), 0.6f, [this] { Shot(TEXT("01_start")); });

			// Puzzle 1: memo, one wrong frame, wait for both hints, then the latest frame.
			Add(TEXT("inspect memo"), 0.4f, [this] { if (AEscapeInteractable* Memo = Find<AEscapeInteractable>(TEXT("memo"))) Press(Memo->Mesh, 1, true); });
			Add(TEXT("shot memo"), 0.3f, [this] { Shot(TEXT("02_memo")); });
			Add(TEXT("lift wrong frame"), 0.8f, [this]
			{
				const AEscapeRoomManager* Manager = GetManager();
				for (AEscapeFrame* Frame : Manager->GetFramesBySize())
				{
					if (Frame != Manager->GetLatestFrame())
					{
						Press(Frame->Canvas);
						break;
					}
				}
			});
			WaitHint(TEXT("wait P1 hint 1"), 1);
			Add(TEXT("shot P1 hint 1"), 0.2f, [this] { if (AEscapeInteractable* Memo = Find<AEscapeInteractable>(TEXT("memo"))) LookAt(Memo->Mesh, true); });
			Add(TEXT("shot"), 0.4f, [this] { Shot(TEXT("03_hint_p1_memo")); });
			WaitHint(TEXT("wait P1 hint 2"), 2);
			Add(TEXT("look at frames"), 0.2f, [this, FrameWall] { StandAndLook(FrameWall - FVector(0.f, 200.f, 0.f), FrameWall - FVector(0.f, 0.f, 15.f)); });
			Add(TEXT("shot"), 0.5f, [this] { Shot(TEXT("04_hint_p1_labels")); });
			Add(TEXT("lift latest frame"), 0.3f, [this] { Press(GetManager()->GetLatestFrame()->Canvas); });
			Add(TEXT("shot key"), 0.5f, [this] { Shot(TEXT("05_key_found")); });

			// Puzzle 2: drawer, a wrong pair, both hints, the right pair.
			Add(TEXT("open drawer"), 1.6f, [this] { if (AEscapeDesk* Desk = Find<AEscapeDesk>()) Press(Desk->Drawer); });
			Add(TEXT("shot drawer"), 0.8f, [this] { Shot(TEXT("06_drawer_palette")); });
			Add(TEXT("pick red"), 0.4f, [this] { if (AEscapePaint* Paint = Find<AEscapePaint>(TEXT("paint_red"))) Press(Paint->Mesh); });
			Add(TEXT("pick blue (wrong pair)"), 0.4f, [this] { if (AEscapePaint* Paint = Find<AEscapePaint>(TEXT("paint_blue"))) Press(Paint->Mesh); });
			WaitHint(TEXT("wait P2 hint 2"), 2);
			Add(TEXT("look at lamp"), 0.2f, [this] { StandAndLook(FVector(60.f, 0.f, 0.f), FVector(300.f, 0.f, 230.f)); });
			Add(TEXT("shot"), 0.4f, [this] { Shot(TEXT("07_hint_p2_lamp")); });
			Add(TEXT("look at paints"), 0.2f, [this] { if (AEscapePaint* Paint = Find<AEscapePaint>(TEXT("paint_blue"))) LookAt(Paint->Mesh); });
			Add(TEXT("shot"), 0.4f, [this] { Shot(TEXT("08_hint_p2_paints")); });
			Add(TEXT("pick blue"), 0.3f, [this] { if (AEscapePaint* Paint = Find<AEscapePaint>(TEXT("paint_blue"))) Press(Paint->Mesh); });
			Add(TEXT("pick yellow"), 0.4f, [this] { if (AEscapePaint* Paint = Find<AEscapePaint>(TEXT("paint_yellow"))) Press(Paint->Mesh); });
			Add(TEXT("look at safe"), 0.8f, [this] { StandAndLook(FVector(-60.f, 40.f, 0.f), FVector(-200.f, 220.f, 50.f)); });
			Add(TEXT("shot safe"), 0.5f, [this] { Shot(TEXT("09_safe_revealed")); });
			Add(TEXT("face safe at eye height"), 0.2f, [this] { StandAndLook(FVector(-200.f, 70.f, 0.f), FVector(-200.f, 186.f, 52.f)); });
			Add(TEXT("shot safe front"), 0.5f, [this] { Shot(TEXT("09b_safe_front_clue")); });
			Add(TEXT("frames from room center"), 0.2f, [this, FrameWall] { StandAndLook(FVector(60.f, 30.f, 0.f), FrameWall); });
			Add(TEXT("shot frames"), 0.5f, [this] { Shot(TEXT("09c_frames_from_center")); });

			// Puzzle 3: a wrong code, both hints, then the code from the frames.
			Add(TEXT("pull handle (0000)"), 0.4f, [this] { if (AEscapeSafe* Safe = Find<AEscapeSafe>()) Press(Safe->Handle); });
			WaitHint(TEXT("wait P3 hint 2"), 2);
			Add(TEXT("look at frames"), 0.2f, [this, FrameWall] { StandAndLook(FrameWall - FVector(0.f, 230.f, 0.f), FrameWall); });
			Add(TEXT("shot"), 0.3f, [this] { Shot(TEXT("10_hint_p3_sequence")); });
			Add(TEXT("enter code"), 0.6f, [this]
			{
				AEscapeSafe* Safe = Find<AEscapeSafe>();
				const FString Code = GetManager()->SafeCode;
				for (int32 Dial = 0; Safe && Dial < Safe->Dials.Num() && Dial < Code.Len(); ++Dial)
				{
					const int32 Target = Code[Dial] - TEXT('0');
					Press(Safe->Dials[Dial], (Target - Safe->GetDialValue(Dial) + 10) % 10);
				}
			});
			Add(TEXT("pull handle"), 0.4f, [this] { if (AEscapeSafe* Safe = Find<AEscapeSafe>()) Press(Safe->Handle); });
			Add(TEXT("shot safe open"), 1.0f, [this] { Shot(TEXT("11_safe_open")); });

			// Escape.
			Add(TEXT("open door"), 0.6f, [this] { if (AEscapeDoor* Door = Find<AEscapeDoor>()) Press(Door->Mesh); });
			Add(TEXT("step back"), 0.2f, [this] { StandAndLook(FVector(60.f, 0.f, 0.f), FVector(300.f, 0.f, 120.f)); });
			Add(TEXT("shot escaped"), 1.6f, [this] { Shot(TEXT("12_escaped")); });
			Add(TEXT("done"), 1.0f, [this]
			{
				const AEscapeRoomManager* Manager = GetManager();
				UE_LOG(LogEscapeAutoplay, Display, TEXT("Autoplay finished: escaped=%d"), Manager && Manager->HasEscaped());
			});
		}

		TWeakObjectPtr<UWorld> World;
		bool bQuit = false;
		TArray<FStep> Steps;
		int32 Index = 0;
		bool bStarted = false;
		double StepTime = FPlatformTime::Seconds();
		FTSTicker::FDelegateHandle TickerHandle;
	};

	TUniquePtr<FEscapeAutoplay> GAutoplay;

	void FEscapeAutoplay::Finish()
	{
		const bool bShouldQuit = bQuit;
		// Destroy on the next tick, not from inside our own ticker callback.
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([bShouldQuit](float)
		{
			GAutoplay.Reset();
			if (bShouldQuit)
			{
				RequestEngineExit(TEXT("Escape.Autoplay finished"));
			}
			return false;
		}));
	}

	void StartAutoplay(const TArray<FString>& Args, UWorld* World)
	{
		if (!World || !World->IsGameWorld())
		{
			UE_LOG(LogEscapeAutoplay, Error, TEXT("Escape.Autoplay needs a running game world"));
			return;
		}
		const float HintDelay = Args.Num() > 0 ? FMath::Max(0.5f, FCString::Atof(*Args[0])) : 2.f;
		auto HasArg = [&Args](const TCHAR* Name) { return Args.ContainsByPredicate([Name](const FString& Arg) { return Arg.Equals(Name, ESearchCase::IgnoreCase); }); };
		GAutoplay = MakeUnique<FEscapeAutoplay>(World, HintDelay, HasArg(TEXT("quit")), HasArg(TEXT("ceiling")));
	}

	FAutoConsoleCommandWithWorldAndArgs GEscapeAutoplayCommand(
		TEXT("Escape.Autoplay"),
		TEXT("Plays the escape room start to finish. Args: [HintDelaySeconds=2] [quit] [ceiling = only take look-up screenshots]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&StartAutoplay));
}

#endif
