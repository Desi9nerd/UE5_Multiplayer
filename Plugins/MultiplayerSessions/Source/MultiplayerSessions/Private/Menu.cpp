#include "Menu.h"
#include "Components/Button.h"
#include "MultiplayerSessionsSubsystem.h"

void UMenu::MenuSetup()
{
	AddToViewport();
	SetVisibility(ESlateVisibility::Visible);
	bIsFocusable = true;

	UWorld* World = GetWorld();
	if (World)
	{
		APlayerController* PlayerController = World->GetFirstPlayerController();
		if (PlayerController)
		{
			FInputModeUIOnly InputModeData;
			InputModeData.SetWidgetToFocus(TakeWidget());
			InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			PlayerController->SetInputMode(InputModeData);
			PlayerController->SetShowMouseCursor(true);
		}
	}

	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance)
	{
		//������� ���� MultiplayerSessionsSubsystem ������ Subsystem�� ���� ���� �־��ش�.
		MultiplayerSessionsSubsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
	}
}

bool UMenu::Initialize()
{
	//Super������ false�� false�� �����Ѵ�.(=���� Ŭ�������� Initialize �ϴ°� ������ false�� �����Ͽ� Initialize()�� ���� �ʴ´�.)
	if (!Super::Initialize())
	{
		return false;
	}

	if (HostButton)
	{
		//HostButton Ŭ���� �� �߻��ϴ� �̺�Ʈ�� Delegate�� �������ش�.
		HostButton->OnClicked.AddDynamic(this, &ThisClass::HostButtonClicked);
	}
	if (JoinButton)
	{
		//JoinButton Ŭ���� �� �߻��ϴ� �̺�Ʈ�� Delegate�� �������ش�.
		JoinButton->OnClicked.AddDynamic(this, &ThisClass::JoinButtonClicked);
	}

	return true;
}

void UMenu::HostButtonClicked()
{
	//����� �׽�Ʈ
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			15.f,
			FColor::Yellow,
			FString(TEXT("Host Button Clicked"))
		);
	}

	if (MultiplayerSessionsSubsystem)
	{
		MultiplayerSessionsSubsystem->CreateSession(4, FString("FreeForAll"));
	}
}

void UMenu::JoinButtonClicked()
{
	//����� �׽�Ʈ
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			15.f,
			FColor::Yellow,
			FString(TEXT("Join Button Clicked"))
		);
	}
}