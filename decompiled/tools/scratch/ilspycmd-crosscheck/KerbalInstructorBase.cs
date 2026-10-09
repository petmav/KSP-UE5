using System;
using System.Collections.Generic;
using UnityEngine;

public class KerbalInstructorBase : MonoBehaviour
{
	public Camera instructorCamera;

	public Material PortraitRenderMaterial;

	public string CharacterName = "";

	public Transform AnimationRoot;

	public Animation anim;

	protected Animator animator;

	protected bool isUsingAnimator;

	[NonSerialized]
	protected List<CharacterAnimationState> anims;

	[NonSerialized]
	protected CharacterAnimationState currentEmote;

	protected AudioSource _audioSource;

	protected void Start()
	{
		SetupAnimations();
	}

	public void SetupCamera(RenderTexture rt)
	{
		instructorCamera.targetTexture = rt;
		instructorCamera.ResetAspect();
		instructorCamera.enabled = true;
	}

	public void ClearCamera()
	{
		if (instructorCamera != null)
		{
			if (instructorCamera.targetTexture != null)
			{
				instructorCamera.targetTexture.DiscardContents();
				instructorCamera.targetTexture.Release();
				instructorCamera.targetTexture = null;
			}
			instructorCamera.enabled = false;
		}
	}

	private void OnDestroy()
	{
		ClearCamera();
	}

	[ContextMenu("Reset Animations")]
	public void SetupAnimations()
	{
		anims = CharacterAnimationUtil.GetAnimationsListFromScript(this);
		CharacterAnimationUtil.SetupAnimations(anims, AnimationRoot);
		animator = AnimationRoot.GetComponent<Animator>();
		isUsingAnimator = animator != null;
		anim = AnimationRoot.GetComponent<Animation>();
	}
}
