using System.Collections.Generic;
using System.Xml.Xsl.Runtime;

namespace System.Xml.Xsl.Qil;

internal class QilExpression : QilNode
{
	private QilFactory factory;

	private QilNode isDebug;

	private QilNode defWSet;

	private QilNode wsRules;

	private QilNode gloVars;

	private QilNode gloParams;

	private QilNode earlBnd;

	private QilNode funList;

	private QilNode rootNod;

	public override int Count => 8;

	public override QilNode this[int index]
	{
		get
		{
			return index switch
			{
				0 => isDebug, 
				1 => defWSet, 
				2 => wsRules, 
				3 => gloParams, 
				4 => gloVars, 
				5 => earlBnd, 
				6 => funList, 
				7 => rootNod, 
				_ => throw new IndexOutOfRangeException(), 
			};
		}
		set
		{
			switch (index)
			{
			case 0:
				isDebug = value;
				break;
			case 1:
				defWSet = value;
				break;
			case 2:
				wsRules = value;
				break;
			case 3:
				gloParams = value;
				break;
			case 4:
				gloVars = value;
				break;
			case 5:
				earlBnd = value;
				break;
			case 6:
				funList = value;
				break;
			case 7:
				rootNod = value;
				break;
			default:
				throw new IndexOutOfRangeException();
			}
		}
	}

	public QilFactory Factory
	{
		get
		{
			return factory;
		}
		set
		{
			factory = value;
		}
	}

	public bool IsDebug
	{
		get
		{
			return isDebug.NodeType == QilNodeType.True;
		}
		set
		{
			isDebug = (value ? factory.True() : factory.False());
		}
	}

	public XmlWriterSettings DefaultWriterSettings
	{
		get
		{
			return (XmlWriterSettings)((QilLiteral)defWSet).Value;
		}
		set
		{
			value.ReadOnly = true;
			((QilLiteral)defWSet).Value = value;
		}
	}

	public IList<WhitespaceRule> WhitespaceRules
	{
		get
		{
			return (IList<WhitespaceRule>)((QilLiteral)wsRules).Value;
		}
		set
		{
			((QilLiteral)wsRules).Value = value;
		}
	}

	public QilList GlobalParameterList
	{
		get
		{
			return (QilList)gloParams;
		}
		set
		{
			gloParams = value;
		}
	}

	public QilList GlobalVariableList
	{
		get
		{
			return (QilList)gloVars;
		}
		set
		{
			gloVars = value;
		}
	}

	public IList<EarlyBoundInfo> EarlyBoundTypes
	{
		get
		{
			return (IList<EarlyBoundInfo>)((QilLiteral)earlBnd).Value;
		}
		set
		{
			((QilLiteral)earlBnd).Value = value;
		}
	}

	public QilList FunctionList
	{
		get
		{
			return (QilList)funList;
		}
		set
		{
			funList = value;
		}
	}

	public QilNode Root
	{
		get
		{
			return rootNod;
		}
		set
		{
			rootNod = value;
		}
	}

	public QilExpression(QilNodeType nodeType, QilNode root)
		: this(nodeType, root, new QilFactory())
	{
	}

	public QilExpression(QilNodeType nodeType, QilNode root, QilFactory factory)
		: base(nodeType)
	{
		this.factory = factory;
		isDebug = factory.False();
		defWSet = factory.LiteralObject(new XmlWriterSettings
		{
			ConformanceLevel = ConformanceLevel.Auto
		});
		wsRules = factory.LiteralObject(new List<WhitespaceRule>());
		gloVars = factory.GlobalVariableList();
		gloParams = factory.GlobalParameterList();
		earlBnd = factory.LiteralObject(new List<EarlyBoundInfo>());
		funList = factory.FunctionList();
		rootNod = root;
	}
}
