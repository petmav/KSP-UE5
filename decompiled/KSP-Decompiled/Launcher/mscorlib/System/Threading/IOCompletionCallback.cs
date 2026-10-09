using System.Runtime.InteropServices;
using System.Security;

namespace System.Threading;

[ComVisible(true)]
[CLSCompliant(false)]
[SecurityCritical]
public unsafe delegate void IOCompletionCallback(uint errorCode, uint numBytes, NativeOverlapped* pOVERLAP);
