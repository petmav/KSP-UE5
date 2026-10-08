using System.ComponentModel;
using System.Diagnostics;
using System.Text;
using Unity;

namespace System.IO.Ports;

[MonitoringDescription("SerialPortDesc")]
public class SerialPort : Component
{
	public const int InfiniteTimeout = -1;

	public Stream BaseStream
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public int BaudRate
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public bool BreakState
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public int BytesToRead
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
	}

	public int BytesToWrite
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
	}

	public bool CDHolding
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
	}

	public bool CtsHolding
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
	}

	public int DataBits
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public bool DiscardNull
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public bool DsrHolding
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
	}

	public bool DtrEnable
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public Encoding Encoding
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public Handshake Handshake
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(Handshake);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public bool IsOpen
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
	}

	public string NewLine
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public Parity Parity
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(Parity);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public byte ParityReplace
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(byte);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public string PortName
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public int ReadBufferSize
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public int ReadTimeout
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public int ReceivedBytesThreshold
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public bool RtsEnable
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public StopBits StopBits
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(StopBits);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public int WriteBufferSize
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public int WriteTimeout
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public event SerialDataReceivedEventHandler DataReceived
	{
		add
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
		remove
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public event SerialErrorReceivedEventHandler ErrorReceived
	{
		add
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
		remove
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public event SerialPinChangedEventHandler PinChanged
	{
		add
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
		remove
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public SerialPort()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public SerialPort(IContainer container)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public SerialPort(string portName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public SerialPort(string portName, int baudRate)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public SerialPort(string portName, int baudRate, Parity parity)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public SerialPort(string portName, int baudRate, Parity parity, int dataBits)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public SerialPort(string portName, int baudRate, Parity parity, int dataBits, StopBits stopBits)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Close()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DiscardInBuffer()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DiscardOutBuffer()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public static string[] GetPortNames()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public void Open()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public int Read(byte[] buffer, int offset, int count)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	public int Read(char[] buffer, int offset, int count)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	public int ReadByte()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	public int ReadChar()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	public string ReadExisting()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public string ReadLine()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public string ReadTo(string value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public void Write(byte[] buffer, int offset, int count)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Write(char[] buffer, int offset, int count)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Write(string text)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void WriteLine(string text)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
