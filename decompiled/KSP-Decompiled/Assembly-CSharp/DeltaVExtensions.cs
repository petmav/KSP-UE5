public static class DeltaVExtensions
{
	public static T GetSwitchedValue<T>(this DeltaVSituationOptions situation, T asl, T actual, T vac)
	{
		switch (situation)
		{
		case DeltaVSituationOptions.SeaLevel:
			return asl;
		case DeltaVSituationOptions.Altitude:
			return actual;
		default:
			return vac;
		}
	}
}
