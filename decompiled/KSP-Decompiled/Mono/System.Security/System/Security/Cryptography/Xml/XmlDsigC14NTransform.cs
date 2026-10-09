using System.IO;
using System.Xml;

namespace System.Security.Cryptography.Xml;

public class XmlDsigC14NTransform : Transform
{
	private Type[] _inputTypes = new Type[3]
	{
		typeof(Stream),
		typeof(XmlDocument),
		typeof(XmlNodeList)
	};

	private Type[] _outputTypes = new Type[1] { typeof(Stream) };

	private CanonicalXml _cXml;

	private bool _includeComments;

	public override Type[] InputTypes => _inputTypes;

	public override Type[] OutputTypes => _outputTypes;

	public XmlDsigC14NTransform()
	{
		base.Algorithm = "http://www.w3.org/TR/2001/REC-xml-c14n-20010315";
	}

	public XmlDsigC14NTransform(bool includeComments)
	{
		_includeComments = includeComments;
		base.Algorithm = (includeComments ? "http://www.w3.org/TR/2001/REC-xml-c14n-20010315#WithComments" : "http://www.w3.org/TR/2001/REC-xml-c14n-20010315");
	}

	public override void LoadInnerXml(XmlNodeList nodeList)
	{
	}

	protected override XmlNodeList GetInnerXml()
	{
		return null;
	}

	public override void LoadInput(object obj)
	{
		XmlResolver resolver = (base.ResolverSet ? _xmlResolver : new XmlSecureResolver(new XmlUrlResolver(), base.BaseURI));
		if (obj is Stream)
		{
			_cXml = new CanonicalXml((Stream)obj, _includeComments, resolver, base.BaseURI);
			return;
		}
		if (obj is XmlDocument)
		{
			_cXml = new CanonicalXml((XmlDocument)obj, resolver, _includeComments);
			return;
		}
		if (obj is XmlNodeList)
		{
			_cXml = new CanonicalXml((XmlNodeList)obj, resolver, _includeComments);
			return;
		}
		throw new ArgumentException("Type of input object is invalid.", "obj");
	}

	public override object GetOutput()
	{
		return new MemoryStream(_cXml.GetBytes());
	}

	public override object GetOutput(Type type)
	{
		if (type != typeof(Stream) && !type.IsSubclassOf(typeof(Stream)))
		{
			throw new ArgumentException("The input type was invalid for this transform.", "type");
		}
		return new MemoryStream(_cXml.GetBytes());
	}

	public override byte[] GetDigestedOutput(HashAlgorithm hash)
	{
		return _cXml.GetDigestedBytes(hash);
	}
}
