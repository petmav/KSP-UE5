using System.Runtime.InteropServices;

namespace System;

[Serializable]
[ComVisible(true)]
public delegate void EventHandler(object sender, EventArgs e);
[Serializable]
public delegate void EventHandler<TEventArgs>(object sender, TEventArgs e);
