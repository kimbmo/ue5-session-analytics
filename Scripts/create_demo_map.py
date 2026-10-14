"""Creates /Game/Maps/AnalyticsDemo containing one AnalyticsDemoActor. Run with the editor's Python plugin."""
import unreal

MAP_PATH = "/Game/Maps/AnalyticsDemo"

level_editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

if not level_editor.new_level(MAP_PATH):
    raise RuntimeError("new_level failed for " + MAP_PATH)

actor_class = unreal.load_class(None, "/Script/SessionAnalyticsDemo.AnalyticsDemoActor")
if actor_class is None:
    raise RuntimeError("AnalyticsDemoActor class not found")

actor = actor_subsystem.spawn_actor_from_class(actor_class, unreal.Vector(0.0, 0.0, 0.0))
if actor is None:
    raise RuntimeError("Could not spawn AnalyticsDemoActor")

if not level_editor.save_current_level():
    raise RuntimeError("save_current_level failed")

unreal.log("DEMO_MAP_CREATED " + MAP_PATH + " actor=" + actor.get_name())
