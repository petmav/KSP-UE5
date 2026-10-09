using System.IO;
using System.Xml;
using System.Xml.XPath;
using System.Xml.Xsl;

namespace System.Security.Cryptography.Xml;

public class XmlDsigXsltTransform : Transform
{
	private Type[] _inputTypes = new Type[3]
	{
		typeof(Stream),
		typeof(XmlDocument),
		typeof(XmlNodeList)
	};

	private Type[] _outputTypes = new Type[1] { typeof(Stream) };

	private XmlNodeList _xslNodes;

	private string _xslFragment;

	private Stream _inputStream;

	private bool _includeComments;

	public override Type[] InputTypes => _inputTypes;

	public override Type[] OutputTypes => _outputTypes;

	public XmlDsigXsltTransform()
	{
		base.Algorithm = "http://www.w3.org/TR/1999/REC-xslt-19991116";
	}

	public XmlDsigXsltTransform(bool includeComments)
	{
		_includeComments = includeComments;
		base.Algorithm = "http://www.w3.org/TR/1999/REC-xslt-19991116";
	}

	public override void LoadInnerXml(XmlNodeList nodeList)
	{
		if (nodeList == null)
		{
			throw new CryptographicException("Unknown transform has been encountered.");
		}
		XmlElement xmlElement = null;
		int num = 0;
		foreach (XmlNode node in nodeList)
		{
			if (node is XmlWhitespace)
			{
				continue;
			}
			if (node is XmlElement)
			{
				if (num != 0)
				{
					throw new CryptographicException("Unknown transform has been encountered.");
				}
				xmlElement = node as XmlElement;
				num++;
			}
			else
			{
				num++;
			}
		}
		if (num != 1 || xmlElement == null)
		{
			throw new CryptographicException("Unknown transform has been encountered.");
		}
		_xslNodes = nodeList;
		_xslFragment = xmlElement.OuterXml.Trim(null);
	}

	protected override XmlNodeList GetInnerXml()
	{
		return _xslNodes;
	}

	public override void LoadInput(object obj)
	{
		if (_inputStream != null)
		{
			_inputStream.Close();
		}
		_inputStream = new MemoryStream();
		if (obj is Stream)
		{
			_inputStream = (Stream)obj;
		}
		else if (obj is XmlNodeList)
		{
			byte[] bytes = new CanonicalXml((XmlNodeList)obj, null, _includeComments).GetBytes();
			if (bytes != null)
			{
				_inputStream.Write(bytes, 0, bytes.Length);
				_inputStream.Flush();
				_inputStream.Position = 0L;
			}
		}
		else if (obj is XmlDocument)
		{
			byte[] bytes2 = new CanonicalXml((XmlDocument)obj, null, _includeComments).GetBytes();
			if (bytes2 != null)
			{
				_inputStream.Write(bytes2, 0, bytes2.Length);
				_inputStream.Flush();
				_inputStream.Position = 0L;
			}
		}
	}

	public override object GetOutput()
	{
		XslCompiledTransform xslCompiledTransform = new XslCompiledTransform();
		XmlReaderSettings xmlReaderSettings = new XmlReaderSettings();
		xmlReaderSettings.XmlResolver = null;
		xmlReaderSettings.MaxCharactersFromEntities = 10000000L;
		xmlReaderSettings.MaxCharactersInDocument = 0L;
		using StringReader input = new StringReader(_xslFragment);
		XmlReader stylesheet = XmlReader.Create((TextReader)input, xmlReaderSettings, (string)null);
		xslCompiledTransform.Load(stylesheet, XsltSettings.Default, null);
		XPathDocument input2 = new XPathDocument(XmlReader.Create(_inputStream, xmlReaderSettings, base.BaseURI), XmlSpace.Preserve);
		MemoryStream memoryStream = new MemoryStream();
		XmlWriter results = new XmlTextWriter(memoryStream, null);
		xslCompiledTransform.Transform(input2, null, results);
		memoryStream.Position = 0L;
		return memoryStream;
	}

	public override object GetOutput(Type type)
	{
		if (type != typeof(Stream) && !type.IsSubclassOf(typeof(Stream)))
		{
			throw new ArgumentException("The input type was invalid for this transform.", "type");
		}
		return (Stream)GetOutput();
	}
}
