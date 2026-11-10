#include "Crowd/DhCrowdSettings.h"

#include "Crowd/DhCrowdCharacter.h"

UDhCrowdSettings::UDhCrowdSettings()
{
	CharacterClass = ADhCrowdCharacter::StaticClass();

	LaneFolders = { TEXT("Content/Crowd/Lanes"), TEXT("Data/Crowd/Lanes") };

	// Manny (male stand-in) and Quinn (female stand-in). setup_crowd.py copies these out of the engine's
	// MoverExamples plugin content into /Game/Crowd/Mannequin; the other candidates are tried afterwards.
	FDhCrowdVisualSet Manny;
	Manny.Name = TEXT("Manny");
	Manny.Mesh = FSoftObjectPath(TEXT("/Game/Crowd/Mannequin/SKM_Manny_Simple.SKM_Manny_Simple"));
	Manny.Walk = FSoftObjectPath(TEXT("/Game/Crowd/Mannequin/MM_Walk_InPlace.MM_Walk_InPlace"));
	Manny.Idle = FSoftObjectPath(TEXT("/Game/Crowd/Mannequin/MM_Idle.MM_Idle"));
	Manny.bFemale = false;
	VisualSets.Add(Manny);

	FDhCrowdVisualSet Quinn;
	Quinn.Name = TEXT("Quinn");
	Quinn.Mesh = FSoftObjectPath(TEXT("/Game/Crowd/Mannequin/SKM_Quinn_Simple.SKM_Quinn_Simple"));
	Quinn.Walk = FSoftObjectPath(TEXT("/Game/Crowd/Mannequin/MM_Walk_InPlace.MM_Walk_InPlace"));
	Quinn.Idle = FSoftObjectPath(TEXT("/Game/Crowd/Mannequin/MM_Idle.MM_Idle"));
	Quinn.bFemale = true;
	VisualSets.Add(Quinn);

	FDhCrowdVisualSet MannyThirdPerson;
	MannyThirdPerson.Name = TEXT("Manny_ThirdPersonTemplate");
	MannyThirdPerson.Mesh = FSoftObjectPath(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
	MannyThirdPerson.Walk = FSoftObjectPath(TEXT("/Game/Characters/Mannequins/Animations/Manny/MM_Walk_Fwd.MM_Walk_Fwd"));
	MannyThirdPerson.Idle = FSoftObjectPath(TEXT("/Game/Characters/Mannequins/Animations/Manny/MM_Idle.MM_Idle"));
	MannyThirdPerson.bFemale = false;
	VisualSets.Add(MannyThirdPerson);

	FDhCrowdVisualSet MannyMover;
	MannyMover.Name = TEXT("Manny_MoverExamples");
	MannyMover.Mesh = FSoftObjectPath(TEXT("/MoverExamples/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
	MannyMover.Walk = FSoftObjectPath(TEXT("/MoverExamples/Characters/Mannequins/Animations/Manny/MM_Walk_InPlace.MM_Walk_InPlace"));
	MannyMover.Idle = FSoftObjectPath(TEXT("/MoverExamples/Characters/Mannequins/Animations/Manny/MM_Idle.MM_Idle"));
	MannyMover.bFemale = false;
	VisualSets.Add(MannyMover);
}
