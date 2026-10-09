using System.Collections;
using System.Xml.Xsl.Runtime;

namespace System.Xml.Xsl;

internal class XmlILCommand
{
	private ExecuteDelegate delExec;

	private XmlQueryStaticData staticData;

	public ExecuteDelegate ExecuteDelegate => delExec;

	public XmlQueryStaticData StaticData => staticData;

	public XmlILCommand(ExecuteDelegate delExec, XmlQueryStaticData staticData)
	{
		this.delExec = delExec;
		this.staticData = staticData;
	}

	public IList Evaluate(string contextDocumentUri, XmlResolver dataSources, XsltArgumentList argumentList)
	{
		XmlCachedSequenceWriter xmlCachedSequenceWriter = new XmlCachedSequenceWriter();
		Execute(contextDocumentUri, dataSources, argumentList, xmlCachedSequenceWriter);
		return xmlCachedSequenceWriter.ResultSequence;
	}

	public void Execute(object defaultDocument, XmlResolver dataSources, XsltArgumentList argumentList, XmlWriter writer)
	{
		try
		{
			if (writer is XmlAsyncCheckWriter)
			{
				writer = ((XmlAsyncCheckWriter)writer).CoreWriter;
			}
			if (writer is XmlWellFormedWriter { RawWriter: not null, WriteState: WriteState.Start } xmlWellFormedWriter && xmlWellFormedWriter.Settings.ConformanceLevel != ConformanceLevel.Document)
			{
				Execute(defaultDocument, dataSources, argumentList, new XmlMergeSequenceWriter(xmlWellFormedWriter.RawWriter));
			}
			else
			{
				Execute(defaultDocument, dataSources, argumentList, new XmlMergeSequenceWriter(new XmlRawWriterWrapper(writer)));
			}
		}
		finally
		{
			writer.Flush();
		}
	}

	private void Execute(object defaultDocument, XmlResolver dataSources, XsltArgumentList argumentList, XmlSequenceWriter results)
	{
		if (dataSources == null)
		{
			dataSources = XmlNullResolver.Singleton;
		}
		delExec(new XmlQueryRuntime(staticData, defaultDocument, dataSources, argumentList, results));
	}
}
