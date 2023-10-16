#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Announcement.generated.h"

/**
 * 
 */
UCLASS()
class MULTIPLAYER_API UAnnouncement : public UUserWidget
{
	GENERATED_BODY()

public:

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* WarmupTime; // ��� ���� �������� ���ð�

	UPROPERTY(meta = (BindWidget))
	UTextBlock* AnnouncementText; 

	UPROPERTY(meta = (BindWidget))
	UTextBlock* InfoText;
};
