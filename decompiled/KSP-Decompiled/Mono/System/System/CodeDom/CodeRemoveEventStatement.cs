namespace System.CodeDom;

[Serializable]
public class CodeRemoveEventStatement : CodeStatement
{
	private CodeEventReferenceExpression _eventRef;

	public CodeEventReferenceExpression Event
	{
		get
		{
			return _eventRef ?? (_eventRef = new CodeEventReferenceExpression());
		}
		set
		{
			_eventRef = value;
		}
	}

	public CodeExpression Listener { get; set; }

	public CodeRemoveEventStatement()
	{
	}

	public CodeRemoveEventStatement(CodeEventReferenceExpression eventRef, CodeExpression listener)
	{
		_eventRef = eventRef;
		Listener = listener;
	}

	public CodeRemoveEventStatement(CodeExpression targetObject, string eventName, CodeExpression listener)
	{
		_eventRef = new CodeEventReferenceExpression(targetObject, eventName);
		Listener = listener;
	}
}
