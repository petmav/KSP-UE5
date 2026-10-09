using System;
using System.Collections;
using System.Globalization;
using System.Threading;
using System.Xml;
using System.Xml.XPath;

namespace MS.Internal.Xml.XPath;

internal sealed class XPathComparerHelper : IComparer
{
	private XmlSortOrder order;

	private XmlCaseOrder caseOrder;

	private CultureInfo cinfo;

	private XmlDataType dataType;

	public XPathComparerHelper(XmlSortOrder order, XmlCaseOrder caseOrder, string lang, XmlDataType dataType)
	{
		if (lang == null)
		{
			cinfo = Thread.CurrentThread.CurrentCulture;
		}
		else
		{
			try
			{
				cinfo = new CultureInfo(lang);
			}
			catch (ArgumentException)
			{
				throw;
			}
		}
		if (order == XmlSortOrder.Descending)
		{
			switch (caseOrder)
			{
			case XmlCaseOrder.LowerFirst:
				caseOrder = XmlCaseOrder.UpperFirst;
				break;
			case XmlCaseOrder.UpperFirst:
				caseOrder = XmlCaseOrder.LowerFirst;
				break;
			}
		}
		this.order = order;
		this.caseOrder = caseOrder;
		this.dataType = dataType;
	}

	public int Compare(object x, object y)
	{
		switch (dataType)
		{
		case XmlDataType.Text:
		{
			string strA = Convert.ToString(x, cinfo);
			string strB = Convert.ToString(y, cinfo);
			int num2 = string.Compare(strA, strB, caseOrder != XmlCaseOrder.None, cinfo);
			if (num2 != 0 || caseOrder == XmlCaseOrder.None)
			{
				if (order != XmlSortOrder.Ascending)
				{
					return -num2;
				}
				return num2;
			}
			num2 = string.Compare(strA, strB, ignoreCase: false, cinfo);
			if (caseOrder != XmlCaseOrder.LowerFirst)
			{
				return -num2;
			}
			return num2;
		}
		case XmlDataType.Number:
		{
			double num = XmlConvert.ToXPathDouble(x);
			double value = XmlConvert.ToXPathDouble(y);
			int num2 = num.CompareTo(value);
			if (order != XmlSortOrder.Ascending)
			{
				return -num2;
			}
			return num2;
		}
		default:
			throw new InvalidOperationException(Res.GetString("Operation is not valid due to the current state of the object."));
		}
	}
}
