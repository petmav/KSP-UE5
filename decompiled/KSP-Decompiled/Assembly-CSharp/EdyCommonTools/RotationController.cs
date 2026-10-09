using System;
using UnityEngine;
using UnityEngine.Serialization;

namespace EdyCommonTools
{
	public class RotationController : MonoBehaviour
	{
		public delegate void OnRotationFinished();

		[Serializable]
		public class Rotation
		{
			public enum Axis
			{
				Up,
				Right,
				Forward
			}

			public enum Mode
			{
				Disabled,
				Free,
				LookAtTarget,
				LookAtTargetCenter
			}

			public float angle;

			public Mode mode = Mode.Free;

			public float targetOffset;

			public bool damped = true;

			public float damping = 4f;

			public bool clamped;

			public float minAngle = -120f;

			public float maxAngle = 120f;

			private float m_currentAngle;

			private float m_currentTarget;

			private Axis m_axis;

			private Vector3 m_axisVector;

			public Quaternion rotation
			{
				get
				{
					if (mode == Mode.Disabled)
					{
						return Quaternion.identity;
					}
					return Quaternion.AngleAxis(m_currentAngle, m_axisVector);
				}
			}

			public bool isRotating
			{
				get
				{
					if (mode != 0)
					{
						return Mathf.Abs(m_currentAngle - m_currentTarget) < 0.001f;
					}
					return false;
				}
			}

			public Rotation(Axis axis)
			{
				m_axis = axis;
				m_axisVector = GetAxisVector(axis);
				mode = Mode.Disabled;
			}

			public Rotation(Axis axis, Mode rotationMode)
			{
				m_axis = axis;
				m_axisVector = GetAxisVector(axis);
				mode = rotationMode;
			}

			public Rotation(Axis axis, Mode rotationMode, float min, float max)
			{
				m_axis = axis;
				m_axisVector = GetAxisVector(axis);
				mode = rotationMode;
				clamped = true;
				minAngle = min;
				maxAngle = max;
			}

			public void ResetAngle(float newAngle)
			{
				m_currentAngle = newAngle;
				m_currentTarget = newAngle;
				angle = newAngle;
			}

			public void Update(float deltaTime, Transform self, Transform target, Renderer targetRenderer, Transform reference)
			{
				if (mode == Mode.Disabled)
				{
					return;
				}
				if (clamped)
				{
					angle = Mathf.Clamp(angle, minAngle, maxAngle);
				}
				float num = angle;
				if (target != null && mode > Mode.Free)
				{
					if (mode == Mode.LookAtTarget)
					{
						num = AngleToTarget(self.position, target.position, reference) + targetOffset;
					}
					else if (mode == Mode.LookAtTargetCenter)
					{
						Vector3 targetPos = ((targetRenderer != null) ? targetRenderer.bounds.center : target.position);
						num = AngleToTarget(self.position, targetPos, reference) + targetOffset;
					}
					if (clamped)
					{
						num = Mathf.Clamp(num, minAngle, maxAngle);
					}
				}
				if (damped)
				{
					if (mode == Mode.Free)
					{
						m_currentAngle = Mathf.Lerp(m_currentAngle, num, damping * deltaTime);
					}
					else
					{
						m_currentAngle = Mathf.LerpAngle(m_currentAngle, num, damping * deltaTime);
					}
				}
				else
				{
					m_currentAngle = num;
				}
				m_currentTarget = num;
			}

			private float AngleToTarget(Vector3 pos, Vector3 targetPos, Transform reference)
			{
				Vector3 vector = targetPos - pos;
				Vector3 vector2 = ((reference != null) ? reference.InverseTransformDirection(vector) : vector);
				float num = ((m_axis == Axis.Up) ? Mathf.Atan2(vector2.x, vector2.z) : ((m_axis != Axis.Right) ? angle : Mathf.Atan2(vector2.y, Mathf.Sqrt(vector2.x * vector2.x + vector2.z * vector2.z))));
				return num * 57.29578f;
			}

			private Vector3 GetAxisVector(Axis axis)
			{
				switch (axis)
				{
				case Axis.Up:
					return Vector3.up;
				case Axis.Right:
					return Vector3.left;
				default:
					return Vector3.forward;
				}
			}
		}

		public Transform target;

		public bool rotateInWorldSpace;

		public bool invertHVorder;

		public Rotation horizontal = new Rotation(Rotation.Axis.Up, Rotation.Mode.Free);

		public Rotation vertical = new Rotation(Rotation.Axis.Right, Rotation.Mode.Free, -60f, 60f);

		[FormerlySerializedAs("pitch")]
		public Rotation roll = new Rotation(Rotation.Axis.Forward);

		public OnRotationFinished onRotationFinished;

		private Transform m_trans;

		private Transform m_cachedTarget;

		private Renderer m_targetRenderer;

		private bool m_rotating;

		private void OnEnable()
		{
			m_trans = GetComponent<Transform>();
			m_rotating = false;
			Vector3 vector = (rotateInWorldSpace ? m_trans.rotation.eulerAngles : m_trans.localRotation.eulerAngles);
			if (horizontal.mode == Rotation.Mode.Free)
			{
				horizontal.ResetAngle(MathUtility.ClampAngle(vector.y));
			}
			if (vertical.mode == Rotation.Mode.Free)
			{
				vertical.ResetAngle(0f - MathUtility.ClampAngle(vector.x));
			}
			if (roll.mode == Rotation.Mode.Free)
			{
				roll.ResetAngle(MathUtility.ClampAngle(vector.z));
			}
		}

		private void LateUpdate()
		{
			UpdateTransform();
			if (!vertical.isRotating && !horizontal.isRotating && !roll.isRotating)
			{
				if (m_rotating)
				{
					m_rotating = false;
					if (onRotationFinished != null)
					{
						onRotationFinished();
					}
				}
			}
			else
			{
				m_rotating = true;
			}
		}

		public void UpdateTransform()
		{
			if (target != m_cachedTarget)
			{
				m_targetRenderer = ((target != null) ? target.GetComponent<Renderer>() : null);
				m_cachedTarget = target;
			}
			float deltaTime = Time.deltaTime;
			Transform reference = (rotateInWorldSpace ? null : m_trans.parent);
			horizontal.Update(deltaTime, m_trans, target, m_targetRenderer, reference);
			vertical.Update(deltaTime, m_trans, target, m_targetRenderer, reference);
			roll.Update(deltaTime, m_trans, target, m_targetRenderer, reference);
			Quaternion quaternion = default(Quaternion);
			quaternion = ((!invertHVorder) ? (horizontal.rotation * vertical.rotation) : (vertical.rotation * horizontal.rotation));
			quaternion *= roll.rotation;
			if (rotateInWorldSpace)
			{
				m_trans.rotation = quaternion;
			}
			else
			{
				m_trans.localRotation = quaternion;
			}
		}
	}
}
