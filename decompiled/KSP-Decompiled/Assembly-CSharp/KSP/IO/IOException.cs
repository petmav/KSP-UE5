using System;

namespace KSP.IO
{
	public class IOException : Exception
	{
		protected string message;

		protected string source;

		protected string stack;

		public override string Message => message;

		public override string Source => source;

		public override string StackTrace => stack;

		public IOException(string message, string source, string stack)
		{
			this.message = message;
			this.source = source;
			this.stack = stack;
		}
	}
}
