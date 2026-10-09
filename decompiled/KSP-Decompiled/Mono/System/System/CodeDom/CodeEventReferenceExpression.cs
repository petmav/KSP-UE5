namespace System.CodeDom;

[Serializable]
public class CodeEventReferenceExpression : CodeExpression
{
	private string _eventName;

	public CodeExpression TargetObject { get; set; }

	public string EventName
	{
		get
		{
			return _eventName ?? string.Empty;
		}
		set
		{
			_eventName = value;
		}
	}

	public CodeEventReferenceExpression()
	{
	}

	public CodeEventReferenceExpression(CodeExpression targetObject, string eventName)
	{
		TargetObject = targetObject;
		_eventName = eventName;
	}
}
