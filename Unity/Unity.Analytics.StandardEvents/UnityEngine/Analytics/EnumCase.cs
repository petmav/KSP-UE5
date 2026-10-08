namespace UnityEngine.Analytics;

public class EnumCase : AnalyticsEventAttribute
{
	public enum Styles
	{
		None,
		Snake,
		Lower
	}

	public Styles Style;

	public EnumCase(Styles style)
	{
		Style = style;
	}
}
