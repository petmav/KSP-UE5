namespace System.CodeDom;

[Serializable]
public class CodeRegionDirective : CodeDirective
{
	private string _regionText;

	public string RegionText
	{
		get
		{
			return _regionText ?? string.Empty;
		}
		set
		{
			_regionText = value;
		}
	}

	public CodeRegionMode RegionMode { get; set; }

	public CodeRegionDirective()
	{
	}

	public CodeRegionDirective(CodeRegionMode regionMode, string regionText)
	{
		RegionText = regionText;
		RegionMode = regionMode;
	}
}
