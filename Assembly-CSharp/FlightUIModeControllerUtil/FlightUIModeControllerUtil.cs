namespace FlightUIModeControllerUtil
{
	public static class FlightUIModeControllerUtil
	{
		public static int TransitionIndex(this TabAction ta)
		{
			return (int)ta;
		}

		public static string TransitionStateName(this TabAction ta)
		{
			switch (ta)
			{
			default:
				return "";
			case TabAction.EXPAND:
				return "In";
			case TabAction.COLLAPSE:
				return "Out";
			}
		}
	}
}
