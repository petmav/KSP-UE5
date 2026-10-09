namespace System.CodeDom;

[Serializable]
public class CodeAttachEventStatement : CodeStatement
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

	public CodeAttachEventStatement()
	{
	}

	public CodeAttachEventStatement(CodeEventReferenceExpression eventRef, CodeExpression listener)
	{
		_eventRef = eventRef;
		Listener = listener;
	}

	public CodeAttachEventStatement(CodeExpression targetObject, string eventName, CodeExpression listener)
		: this(new CodeEventReferenceExpression(targetObject, eventName), listener)
	{
	}
}
