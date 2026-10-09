using System.Reflection;
using System.Xml.Xsl.Qil;

namespace System.Xml.Xsl.IlGen;

internal class XmlILAnnotation : ListBase<object>
{
	private object annPrev;

	private MethodInfo funcMethod;

	private int argPos;

	private IteratorDescriptor iterInfo;

	private XmlILConstructInfo constrInfo;

	private OptimizerPatterns optPatt;

	public MethodInfo FunctionBinding
	{
		get
		{
			return funcMethod;
		}
		set
		{
			funcMethod = value;
		}
	}

	public int ArgumentPosition
	{
		get
		{
			return argPos;
		}
		set
		{
			argPos = value;
		}
	}

	public IteratorDescriptor CachedIteratorDescriptor
	{
		get
		{
			return iterInfo;
		}
		set
		{
			iterInfo = value;
		}
	}

	public XmlILConstructInfo ConstructInfo
	{
		get
		{
			return constrInfo;
		}
		set
		{
			constrInfo = value;
		}
	}

	public OptimizerPatterns Patterns
	{
		get
		{
			return optPatt;
		}
		set
		{
			optPatt = value;
		}
	}

	public override int Count
	{
		get
		{
			if (annPrev == null)
			{
				return 2;
			}
			return 3;
		}
	}

	public override object this[int index]
	{
		get
		{
			if (annPrev != null)
			{
				if (index == 0)
				{
					return annPrev;
				}
				index--;
			}
			return index switch
			{
				0 => constrInfo, 
				1 => optPatt, 
				_ => throw new IndexOutOfRangeException(), 
			};
		}
		set
		{
			throw new NotSupportedException();
		}
	}

	public static XmlILAnnotation Write(QilNode nd)
	{
		XmlILAnnotation xmlILAnnotation = nd.Annotation as XmlILAnnotation;
		if (xmlILAnnotation == null)
		{
			xmlILAnnotation = (XmlILAnnotation)(nd.Annotation = new XmlILAnnotation(nd.Annotation));
		}
		return xmlILAnnotation;
	}

	private XmlILAnnotation(object annPrev)
	{
		this.annPrev = annPrev;
	}
}
