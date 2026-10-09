using System;

[AttributeUsage(AttributeTargets.Class, AllowMultiple = true)]
public class KSPCapability : Attribute
{
	public string capabilityName = "";

	public string category { get; private set; }

	public KSPCapability(string category)
	{
		this.category = category;
	}
}
