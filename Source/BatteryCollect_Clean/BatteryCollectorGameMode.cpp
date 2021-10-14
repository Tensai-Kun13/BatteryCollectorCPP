// Copyright Epic Games, Inc. All Rights Reserved.

#include "BatteryCollectorGameMode.h"
#include <gameux.h>
#include "BatteryCollectorCharacter.h"
#include "SpawnVolume.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PawnMovementComponent.h"

ABatteryCollectorGameMode::ABatteryCollectorGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPersonCPP/Blueprints/ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}

	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.bCanEverTick = true;

	// base decay rate
	DecayRate = 0.05f;
}

void ABatteryCollectorGameMode::BeginPlay()
{
	Super::BeginPlay();

	// find all spawn volume Actors
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASpawnVolume::StaticClass(), FoundActors);

	for (auto Actor : FoundActors)
	{
		ASpawnVolume* SpawnVolumeActor = Cast<ASpawnVolume>(Actor);
		if (SpawnVolumeActor)
		{
			SpawnVolumeActors.AddUnique(SpawnVolumeActor);
		}
	}

	SetCurrentState(EPlaying);

	// set the score to beat
	ABatteryCollectorCharacter* MyCharacter = Cast<ABatteryCollectorCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (MyCharacter)
	{
		PowerToWin = (MyCharacter->GetInitialPower()) * 1.25f;
	}

	// set the Widget to use
	if (HUDWidgetClass!= nullptr)
	{
		CurrentWidget = CreateWidget<UUserWidget>(GetWorld(), HUDWidgetClass);

		if (CurrentWidget != nullptr)
		{
			CurrentWidget->AddToViewport();
		}
	}
}

void ABatteryCollectorGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Check that we are using the battery collector character
	ABatteryCollectorCharacter* MyCharacter = Cast<ABatteryCollectorCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (MyCharacter)
	{
		// if our power is greater than power needed to win, set the game's state to 'Won'
		if (MyCharacter->GetCurrentPower() > PowerToWin)
		{
			SetCurrentState(EWon);
		}
		// if the character's power is positive
		else if (MyCharacter->GetCurrentPower() > 0)
		{
			// decrease the character's power using the decay rate
			MyCharacter->UpdatePower(-DeltaSeconds*DecayRate*(MyCharacter->GetInitialPower()));
		}
		else
		{
			SetCurrentState(EGameOver);
		}
	}
}

float ABatteryCollectorGameMode::GetPowerToWin() const
{
	return PowerToWin;
}

EBatteryPlayState ABatteryCollectorGameMode::GetCurrentState() const
{
	return CurrentState;
}

void ABatteryCollectorGameMode::SetCurrentState(EBatteryPlayState NewState)
{
	// Set current state
	CurrentState = NewState;
	// Handle the new current state
	HandleNewState(CurrentState);
}

void ABatteryCollectorGameMode::HandleNewState(EBatteryPlayState NewState)
{
	switch (NewState)
	{
		// If the game is playing
		case EPlaying:
			// Spawn volumes active
			for (ASpawnVolume* Volume : SpawnVolumeActors)
			{
				Volume->SetSpawningActive(true);
			}
			break;

		// If we've won the game
		case EWon:
			// Spawn volumes inactive
			for (ASpawnVolume* Volume : SpawnVolumeActors)
			{
				Volume->SetSpawningActive(false);
			}
			break;

		// If we've lost the game
		case EGameOver:
			{
				// Spawn volumes inactive
				for (ASpawnVolume* Volume : SpawnVolumeActors)
				{
					Volume->SetSpawningActive(false);
				}
				// Block player input
				APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
				if (PlayerController)
				{
					PlayerController->SetCinematicMode(true, false, false, true, true);
				}
				// Ragdoll the character
				ACharacter* MyCharacter = UGameplayStatics::GetPlayerCharacter(this, 0);
				if (MyCharacter)
				{
					MyCharacter->GetMesh()->SetSimulatePhysics(true);
					MyCharacter->GetMovementComponent()->MovementState.bCanJump = false;
				}
				break;
			}
		
		// Unknown/default state
		case EUnknown:
		default:
			// do nothing
			break;
	}
}