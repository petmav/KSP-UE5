using UnityEngine;

public class SpaceNavigatorNoDevice : SpaceNavigator
{
	private Vector3 _fakeRotationInput;

	private Vector3 _fakeTranslationInput;

	private const float FakeInputThreshold = 0.1f;

	private static SpaceNavigatorNoDevice _subInstance;

	public static SpaceNavigatorNoDevice SubInstance => _subInstance ?? (_subInstance = new SpaceNavigatorNoDevice());

	public override Vector3 GetTranslation()
	{
		float num = (Application.isPlaying ? PlayTransSens : TransSens[CurrentGear]);
		return new Vector3((SpaceNavigator.LockTranslationX || SpaceNavigator.LockTranslationAll) ? 0f : (_fakeTranslationInput.x * num), (SpaceNavigator.LockTranslationY || SpaceNavigator.LockTranslationAll) ? 0f : (_fakeTranslationInput.y * num), (SpaceNavigator.LockTranslationZ || SpaceNavigator.LockTranslationAll) ? 0f : (_fakeTranslationInput.z * num));
	}

	public override Quaternion GetRotation()
	{
		float num = (Application.isPlaying ? PlayRotSens : RotSens);
		return Quaternion.Euler((SpaceNavigator.LockRotationX || SpaceNavigator.LockRotationAll) ? 0f : (_fakeRotationInput.x * num), (SpaceNavigator.LockRotationY || SpaceNavigator.LockRotationAll) ? 0f : (_fakeRotationInput.y * num), (SpaceNavigator.LockRotationZ || SpaceNavigator.LockRotationAll) ? 0f : (_fakeRotationInput.z * num));
	}

	public override void Dispose()
	{
	}
}
