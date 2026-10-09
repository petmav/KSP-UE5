namespace System.CodeDom;

[Serializable]
public class CodeGotoStatement : CodeStatement
{
	private string _label;

	public string Label
	{
		get
		{
			return _label;
		}
		set
		{
			if (string.IsNullOrEmpty(value))
			{
				throw new ArgumentNullException("value");
			}
			_label = value;
		}
	}

	public CodeGotoStatement()
	{
	}

	public CodeGotoStatement(string label)
	{
		Label = label;
	}
}
