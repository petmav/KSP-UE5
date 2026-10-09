using System;
using System.Diagnostics;
using System.Reflection;
using System.Resources;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Security;
using System.Security.Cryptography;
using System.Security.Permissions;
using System.Threading;

[assembly: SatelliteContractVersion("4.0.0.0")]
[assembly: AllowPartiallyTrustedCallers]
[assembly: DefaultDependency(LoadHint.Always)]
[assembly: SecurityCritical]
[assembly: CLSCompliant(true)]
[assembly: NeutralResourcesLanguage("en-US")]
[assembly: StringFreezing]
[assembly: ComVisible(false)]
[assembly: AssemblyFileVersion("4.6.57.0")]
[assembly: AssemblyInformationalVersion("4.6.57.0")]
[assembly: AssemblyKeyFile("../ecma.pub")]
[assembly: AssemblyCopyright("(c) Various Mono authors")]
[assembly: AssemblyCompany("Mono development team")]
[assembly: AssemblyDefaultAlias("System.Core.dll")]
[assembly: AssemblyDescription("System.Core.dll")]
[assembly: AssemblyTitle("System.Core.dll")]
[assembly: AssemblyProduct("Mono Common Language Infrastructure")]
[assembly: AssemblyDelaySign(true)]
[assembly: AssemblyVersion("4.0.0.0")]
[assembly: TypeForwardedTo(typeof(Action))]
[assembly: TypeForwardedTo(typeof(Action<, >))]
[assembly: TypeForwardedTo(typeof(Action<, , >))]
[assembly: TypeForwardedTo(typeof(Action<, , , >))]
[assembly: TypeForwardedTo(typeof(Func<>))]
[assembly: TypeForwardedTo(typeof(Func<, >))]
[assembly: TypeForwardedTo(typeof(Func<, , >))]
[assembly: TypeForwardedTo(typeof(Func<, , , >))]
[assembly: TypeForwardedTo(typeof(Func<, , , , >))]
[assembly: TypeForwardedTo(typeof(InvalidTimeZoneException))]
[assembly: TypeForwardedTo(typeof(Lazy<>))]
[assembly: TypeForwardedTo(typeof(ExtensionAttribute))]
[assembly: TypeForwardedTo(typeof(Aes))]
[assembly: TypeForwardedTo(typeof(LazyThreadSafetyMode))]
[assembly: TypeForwardedTo(typeof(LockRecursionException))]
[assembly: TypeForwardedTo(typeof(TimeZoneInfo))]
[assembly: TypeForwardedTo(typeof(TimeZoneNotFoundException))]
