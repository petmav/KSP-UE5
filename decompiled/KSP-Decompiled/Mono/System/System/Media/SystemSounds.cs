namespace System.Media;

public sealed class SystemSounds
{
	public static SystemSound Asterisk => new SystemSound("Asterisk");

	public static SystemSound Beep => new SystemSound("Beep");

	public static SystemSound Exclamation => new SystemSound("Exclamation");

	public static SystemSound Hand => new SystemSound("Hand");

	public static SystemSound Question => new SystemSound("Question");

	private SystemSounds()
	{
	}
}
