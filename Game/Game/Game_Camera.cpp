inline vector2
GetCameraSpacePosition(game_state *GameState, entity *Entity)
{
	vector2 Result = Entity->Position - GameState->Camera.Position;

	return (Result);
}

om_internal void
MoveCamera(camera *Camera, world *World, vector2 Position)
{
	//TODO: Choose an educated value for this velocity.
	r32 LerpVelocity = 0.025f;
	vector2 CameraCenter = GetCenter(Camera->CameraWindow);
	vector2 RelativePosition = Position - CameraCenter;

	Camera->Position = Lerp(Camera->Position, RelativePosition, LerpVelocity);

	//TODO: Add "Easy-in" feel for clamping.
	Camera->Position = Clamp(Vector2(0.0f, 0.0f), Camera->Position, Vector2((r32)World->WorldWidth, (r32)World->WorldHeight) - Camera->CameraWindow.Max);
}