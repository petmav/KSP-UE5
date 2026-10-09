using System.Collections;
using System.Xml.Xsl.Qil;

namespace System.Xml.Xsl.IlGen;

internal class XmlILConstructInfo : IQilAnnotation
{
	private QilNodeType nodeType;

	private PossibleXmlStates xstatesInitial;

	private PossibleXmlStates xstatesFinal;

	private PossibleXmlStates xstatesBeginLoop;

	private PossibleXmlStates xstatesEndLoop;

	private bool isNmspInScope;

	private bool mightHaveNmsp;

	private bool mightHaveAttrs;

	private bool mightHaveDupAttrs;

	private bool mightHaveNmspAfterAttrs;

	private XmlILConstructMethod constrMeth;

	private XmlILConstructInfo parentInfo;

	private ArrayList callersInfo;

	private bool isReadOnly;

	private static volatile XmlILConstructInfo Default;

	public PossibleXmlStates InitialStates
	{
		get
		{
			return xstatesInitial;
		}
		set
		{
			xstatesInitial = value;
		}
	}

	public PossibleXmlStates FinalStates
	{
		get
		{
			return xstatesFinal;
		}
		set
		{
			xstatesFinal = value;
		}
	}

	public PossibleXmlStates BeginLoopStates
	{
		set
		{
			xstatesBeginLoop = value;
		}
	}

	public PossibleXmlStates EndLoopStates
	{
		set
		{
			xstatesEndLoop = value;
		}
	}

	public XmlILConstructMethod ConstructMethod
	{
		get
		{
			return constrMeth;
		}
		set
		{
			constrMeth = value;
		}
	}

	public bool PushToWriterFirst
	{
		get
		{
			if (constrMeth != XmlILConstructMethod.Writer)
			{
				return constrMeth == XmlILConstructMethod.WriterThenIterator;
			}
			return true;
		}
		set
		{
			switch (constrMeth)
			{
			case XmlILConstructMethod.Iterator:
				constrMeth = XmlILConstructMethod.WriterThenIterator;
				break;
			case XmlILConstructMethod.IteratorThenWriter:
				constrMeth = XmlILConstructMethod.Writer;
				break;
			}
		}
	}

	public bool PushToWriterLast
	{
		get
		{
			if (constrMeth != XmlILConstructMethod.Writer)
			{
				return constrMeth == XmlILConstructMethod.IteratorThenWriter;
			}
			return true;
		}
		set
		{
			switch (constrMeth)
			{
			case XmlILConstructMethod.Iterator:
				constrMeth = XmlILConstructMethod.IteratorThenWriter;
				break;
			case XmlILConstructMethod.WriterThenIterator:
				constrMeth = XmlILConstructMethod.Writer;
				break;
			}
		}
	}

	public bool PullFromIteratorFirst
	{
		get
		{
			if (constrMeth != XmlILConstructMethod.IteratorThenWriter)
			{
				return constrMeth == XmlILConstructMethod.Iterator;
			}
			return true;
		}
		set
		{
			switch (constrMeth)
			{
			case XmlILConstructMethod.Writer:
				constrMeth = XmlILConstructMethod.IteratorThenWriter;
				break;
			case XmlILConstructMethod.WriterThenIterator:
				constrMeth = XmlILConstructMethod.Iterator;
				break;
			}
		}
	}

	public XmlILConstructInfo ParentInfo
	{
		set
		{
			parentInfo = value;
		}
	}

	public XmlILConstructInfo ParentElementInfo
	{
		get
		{
			if (parentInfo != null && parentInfo.nodeType == QilNodeType.ElementCtor)
			{
				return parentInfo;
			}
			return null;
		}
	}

	public bool IsNamespaceInScope
	{
		get
		{
			return isNmspInScope;
		}
		set
		{
			isNmspInScope = value;
		}
	}

	public bool MightHaveNamespaces
	{
		get
		{
			return mightHaveNmsp;
		}
		set
		{
			mightHaveNmsp = value;
		}
	}

	public bool MightHaveNamespacesAfterAttributes
	{
		get
		{
			return mightHaveNmspAfterAttrs;
		}
		set
		{
			mightHaveNmspAfterAttrs = value;
		}
	}

	public bool MightHaveAttributes
	{
		get
		{
			return mightHaveAttrs;
		}
		set
		{
			mightHaveAttrs = value;
		}
	}

	public bool MightHaveDuplicateAttributes
	{
		get
		{
			return mightHaveDupAttrs;
		}
		set
		{
			mightHaveDupAttrs = value;
		}
	}

	public ArrayList CallersInfo
	{
		get
		{
			if (callersInfo == null)
			{
				callersInfo = new ArrayList();
			}
			return callersInfo;
		}
	}

	public virtual string Name => "ConstructInfo";

	public static XmlILConstructInfo Read(QilNode nd)
	{
		XmlILConstructInfo xmlILConstructInfo = ((nd.Annotation is XmlILAnnotation xmlILAnnotation) ? xmlILAnnotation.ConstructInfo : null);
		if (xmlILConstructInfo == null)
		{
			if (Default == null)
			{
				xmlILConstructInfo = new XmlILConstructInfo(QilNodeType.Unknown);
				xmlILConstructInfo.isReadOnly = true;
				Default = xmlILConstructInfo;
			}
			else
			{
				xmlILConstructInfo = Default;
			}
		}
		return xmlILConstructInfo;
	}

	public static XmlILConstructInfo Write(QilNode nd)
	{
		XmlILAnnotation xmlILAnnotation = XmlILAnnotation.Write(nd);
		XmlILConstructInfo xmlILConstructInfo = xmlILAnnotation.ConstructInfo;
		if (xmlILConstructInfo == null || xmlILConstructInfo.isReadOnly)
		{
			xmlILConstructInfo = (xmlILAnnotation.ConstructInfo = new XmlILConstructInfo(nd.NodeType));
		}
		return xmlILConstructInfo;
	}

	private XmlILConstructInfo(QilNodeType nodeType)
	{
		this.nodeType = nodeType;
		xstatesInitial = (xstatesFinal = PossibleXmlStates.Any);
		xstatesBeginLoop = (xstatesEndLoop = PossibleXmlStates.None);
		isNmspInScope = false;
		mightHaveNmsp = true;
		mightHaveAttrs = true;
		mightHaveDupAttrs = true;
		mightHaveNmspAfterAttrs = true;
		constrMeth = XmlILConstructMethod.Iterator;
		parentInfo = null;
	}

	public override string ToString()
	{
		string text = "";
		if (constrMeth != 0)
		{
			text += constrMeth;
			text = text + ", " + xstatesInitial;
			if (xstatesBeginLoop != 0)
			{
				text = text + " => " + xstatesBeginLoop.ToString() + " => " + xstatesEndLoop;
			}
			text = text + " => " + xstatesFinal;
			if (!MightHaveAttributes)
			{
				text += ", NoAttrs";
			}
			if (!MightHaveDuplicateAttributes)
			{
				text += ", NoDupAttrs";
			}
			if (!MightHaveNamespaces)
			{
				text += ", NoNmsp";
			}
			if (!MightHaveNamespacesAfterAttributes)
			{
				text += ", NoNmspAfterAttrs";
			}
		}
		return text;
	}
}
