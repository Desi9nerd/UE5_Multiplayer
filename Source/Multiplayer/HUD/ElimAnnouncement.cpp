#include "ElimAnnouncement.h"
#include "Components/TextBlock.h"

void UElimAnnouncement::SetElimAnnouncementText(FString AttackerName, FString VictimName)
{
	FString ElimAnnouncementText = FString::Printf(TEXT("%s 이 %s 를 제거 했습니다!"), *AttackerName, *VictimName);
	if (IsValid(AnnouncementText))
	{
		AnnouncementText->SetText(FText::FromString(ElimAnnouncementText));
	}
}
