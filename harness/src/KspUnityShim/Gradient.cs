// Minimal managed stand-in for UnityEngine.Gradient (native-backed in Unity). KSP constructs Gradients in
// field initializers (e.g. PhysicsGlobals.blackBodyRadiation); the harness never evaluates one, so Evaluate
// throws rather than guessing Unity's colour interpolation.
using System;
using UnityEngine;

namespace KspUnityShim
{
    public class Gradient : IEquatable<Gradient>
    {
        private GradientColorKey[] m_ColorKeys = new GradientColorKey[0];
        private GradientAlphaKey[] m_AlphaKeys = new GradientAlphaKey[0];

        public Gradient() { }

        public GradientColorKey[] colorKeys { get { return m_ColorKeys; } set { m_ColorKeys = value; } }
        public GradientAlphaKey[] alphaKeys { get { return m_AlphaKeys; } set { m_AlphaKeys = value; } }
        public GradientMode mode { get; set; }

        public void SetKeys(GradientColorKey[] colorKeys, GradientAlphaKey[] alphaKeys)
        {
            m_ColorKeys = colorKeys; m_AlphaKeys = alphaKeys;
        }

        public Color Evaluate(float time)
        {
            throw new NotSupportedException("Gradient shim: Evaluate is not ported (native Unity code)");
        }

        public override bool Equals(object o) => o is Gradient g && Equals(g);
        public bool Equals(Gradient other) => ReferenceEquals(this, other);
        public override int GetHashCode() => base.GetHashCode();
    }
}
