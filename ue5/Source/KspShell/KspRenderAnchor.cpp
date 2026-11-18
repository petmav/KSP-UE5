#include "KspRenderAnchor.h"

bool FKspRenderAnchor::Update(const FVector& Focus, uint32 FrameChangeSerial)
{
	if (bValid && Serial == FrameChangeSerial && FVector::DistSquared(Focus, Origin) <= ThresholdCm * ThresholdCm)
	{
		return false;
	}
	Origin = Focus;
	Serial = FrameChangeSerial;
	bValid = true;
	++MoveCount;
	return true;
}
