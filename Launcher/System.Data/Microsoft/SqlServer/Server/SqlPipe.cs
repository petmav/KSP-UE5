using System.Data.SqlClient;
using Unity;

namespace Microsoft.SqlServer.Server;

public sealed class SqlPipe
{
	public bool IsSendingResults
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
	}

	internal SqlPipe()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void ExecuteAndSend(SqlCommand command)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Send(SqlDataRecord record)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Send(SqlDataReader reader)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Send(string message)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void SendResultsEnd()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void SendResultsRow(SqlDataRecord record)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void SendResultsStart(SqlDataRecord record)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
