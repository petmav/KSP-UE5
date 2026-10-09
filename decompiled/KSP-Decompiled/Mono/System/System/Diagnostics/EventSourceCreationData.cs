namespace System.Diagnostics;

public class EventSourceCreationData
{
	private string _source;

	private string _logName;

	private string _machineName;

	private string _messageResourceFile;

	private string _parameterResourceFile;

	private string _categoryResourceFile;

	private int _categoryCount;

	public int CategoryCount
	{
		get
		{
			return _categoryCount;
		}
		set
		{
			if (value < 0)
			{
				throw new ArgumentOutOfRangeException("value");
			}
			_categoryCount = value;
		}
	}

	public string CategoryResourceFile
	{
		get
		{
			return _categoryResourceFile;
		}
		set
		{
			_categoryResourceFile = value;
		}
	}

	public string LogName
	{
		get
		{
			return _logName;
		}
		set
		{
			_logName = value;
		}
	}

	public string MachineName
	{
		get
		{
			return _machineName;
		}
		set
		{
			_machineName = value;
		}
	}

	public string MessageResourceFile
	{
		get
		{
			return _messageResourceFile;
		}
		set
		{
			_messageResourceFile = value;
		}
	}

	public string ParameterResourceFile
	{
		get
		{
			return _parameterResourceFile;
		}
		set
		{
			_parameterResourceFile = value;
		}
	}

	public string Source
	{
		get
		{
			return _source;
		}
		set
		{
			_source = value;
		}
	}

	public EventSourceCreationData(string source, string logName)
	{
		_source = source;
		_logName = logName;
		_machineName = ".";
	}

	internal EventSourceCreationData(string source, string logName, string machineName)
	{
		_source = source;
		if (logName == null || logName.Length == 0)
		{
			_logName = "Application";
		}
		else
		{
			_logName = logName;
		}
		_machineName = machineName;
	}
}
