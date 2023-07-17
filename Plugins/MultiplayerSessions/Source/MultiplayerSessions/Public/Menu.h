#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Menu.generated.h"

UCLASS()
class MULTIPLAYERSESSIONS_API UMenu : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
		void MenuSetup(int32 NumberOfPublicConnections = 4, FString TypeOfMatch = FString(TEXT("FreeForAll")));//������ �� �ִ� Player ���ڸ� input���� ����� 4�� �⺻������ �����Ѵ�.

protected:

	virtual bool Initialize() override;
	virtual void OnLevelRemovedFromWorld(ULevel* InLevel, UWorld* InWorld) override;//�ٸ� Level�� �̵��ϸ� ������ Level�� ����� �Լ�.

	//
	// Callbacks for the custom delegates on the MultiplayerSessionsSubsystem
	// MultiplayerSessionsSubsystem.h�� Dynamic Delegate�� ����ȴ�. Dynamic�� ����Ǵ� ��� UFUNCTION()�� �� �ٿ��� �Ѵ�. ������ ������ dynamic delegate�� ������� �ʴ´�.
	UFUNCTION()
		void OnCreateSession(bool bWasSuccessful);

private:
	//meta = (BindWidget)�� ����ϸ� BP�� Button Widget�� �Ʒ��� ������ ����ȴ�. �� �� BP�� C++���� �̸��� ��ġ�Ͽ��� �Ѵ�.
	UPROPERTY(meta = (BindWidget))
		class UButton* HostButton;

	UPROPERTY(meta = (BindWidget))
		UButton* JoinButton;

	UFUNCTION()
		void HostButtonClicked();

	UFUNCTION()
		void JoinButtonClicked();

	void MenuTearDown();

	// The subsystem designed to handle all online session functionality
	class UMultiplayerSessionsSubsystem* MultiplayerSessionsSubsystem;
	
	int32 NumPublicConnections{ 4 };
	FString MatchType{ TEXT("FreeForAll") };
};
