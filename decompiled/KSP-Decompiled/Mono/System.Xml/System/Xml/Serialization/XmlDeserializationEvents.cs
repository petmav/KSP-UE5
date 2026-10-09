namespace System.Xml.Serialization;

public struct XmlDeserializationEvents
{
	private XmlNodeEventHandler onUnknownNode;

	private XmlAttributeEventHandler onUnknownAttribute;

	private XmlElementEventHandler onUnknownElement;

	private UnreferencedObjectEventHandler onUnreferencedObject;

	internal object sender;

	public XmlNodeEventHandler OnUnknownNode
	{
		get
		{
			return onUnknownNode;
		}
		set
		{
			onUnknownNode = value;
		}
	}

	public XmlAttributeEventHandler OnUnknownAttribute
	{
		get
		{
			return onUnknownAttribute;
		}
		set
		{
			onUnknownAttribute = value;
		}
	}

	public XmlElementEventHandler OnUnknownElement
	{
		get
		{
			return onUnknownElement;
		}
		set
		{
			onUnknownElement = value;
		}
	}

	public UnreferencedObjectEventHandler OnUnreferencedObject
	{
		get
		{
			return onUnreferencedObject;
		}
		set
		{
			onUnreferencedObject = value;
		}
	}
}
