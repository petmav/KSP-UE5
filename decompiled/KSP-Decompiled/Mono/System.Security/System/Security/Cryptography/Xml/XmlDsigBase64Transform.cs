using System.IO;
using System.Text;
using System.Xml;

namespace System.Security.Cryptography.Xml;

public class XmlDsigBase64Transform : Transform
{
	private Type[] _inputTypes = new Type[3]
	{
		typeof(Stream),
		typeof(XmlNodeList),
		typeof(XmlDocument)
	};

	private Type[] _outputTypes = new Type[1] { typeof(Stream) };

	private CryptoStream _cs;

	public override Type[] InputTypes => _inputTypes;

	public override Type[] OutputTypes => _outputTypes;

	public XmlDsigBase64Transform()
	{
		base.Algorithm = "http://www.w3.org/2000/09/xmldsig#base64";
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
		if (obj is Stream)
		{
			LoadStreamInput((Stream)obj);
		}
		else if (obj is XmlNodeList)
		{
			LoadXmlNodeListInput((XmlNodeList)obj);
		}
		else if (obj is XmlDocument)
		{
			LoadXmlNodeListInput(((XmlDocument)obj).SelectNodes("//."));
		}
	}

	private void LoadStreamInput(Stream inputStream)
	{
		if (inputStream == null)
		{
			throw new ArgumentException("obj");
		}
		MemoryStream memoryStream = new MemoryStream();
		byte[] array = new byte[1024];
		int num;
		do
		{
			num = inputStream.Read(array, 0, 1024);
			if (num <= 0)
			{
				continue;
			}
			int num2 = 0;
			int i;
			for (i = 0; i < num && !char.IsWhiteSpace((char)array[i]); i++)
			{
			}
			num2 = i;
			for (i++; i < num; i++)
			{
				if (!char.IsWhiteSpace((char)array[i]))
				{
					array[num2] = array[i];
					num2++;
				}
			}
			memoryStream.Write(array, 0, num2);
		}
		while (num > 0);
		memoryStream.Position = 0L;
		_cs = new CryptoStream(memoryStream, new FromBase64Transform(), CryptoStreamMode.Read);
	}

	private void LoadXmlNodeListInput(XmlNodeList nodeList)
	{
		StringBuilder stringBuilder = new StringBuilder();
		foreach (XmlNode node in nodeList)
		{
			XmlNode xmlNode = node.SelectSingleNode("self::text()");
			if (xmlNode != null)
			{
				stringBuilder.Append(xmlNode.OuterXml);
			}
		}
		byte[] bytes = new UTF8Encoding(encoderShouldEmitUTF8Identifier: false).GetBytes(stringBuilder.ToString());
		int num = 0;
		int i;
		for (i = 0; i < bytes.Length && !char.IsWhiteSpace((char)bytes[i]); i++)
		{
		}
		num = i;
		for (i++; i < bytes.Length; i++)
		{
			if (!char.IsWhiteSpace((char)bytes[i]))
			{
				bytes[num] = bytes[i];
				num++;
			}
		}
		MemoryStream stream = new MemoryStream(bytes, 0, num);
		_cs = new CryptoStream(stream, new FromBase64Transform(), CryptoStreamMode.Read);
	}

	public override object GetOutput()
	{
		return _cs;
	}

	public override object GetOutput(Type type)
	{
		if (type != typeof(Stream) && !type.IsSubclassOf(typeof(Stream)))
		{
			throw new ArgumentException("The input type was invalid for this transform.", "type");
		}
		return _cs;
	}
}
