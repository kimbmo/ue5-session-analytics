"""Builds the escape-room prototype content: material, thin Blueprints and the gray-box level /Game/EscapeRoom/L_Studio.

Run headless (editor closed):
  UnrealEditor-Cmd.exe SessionAnalyticsDemo.uproject -run=pythonscript -script="<abs path>/Scripts/create_escape_room.py" -unattended -nosplash

Safe to re-run: Blueprints and materials are reused, the level is rebuilt from scratch.
All gameplay logic lives in C++ (Source/SessionAnalyticsDemo/EscapeRoom); the Blueprints only exist so values can be
tuned in the Details panel.
"""
import unreal

ROOT = "/Game/EscapeRoom"
MAT_DIR = ROOT + "/Materials"
BP_DIR = ROOT + "/Blueprints"
LEVEL = ROOT + "/L_Studio"

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary


def log(msg):
    unreal.log("ESCAPE_ROOM " + msg)


# ------------------------------------------------------------------------------------------------
# Materials

def build_base_material():
    path = MAT_DIR + "/M_EscapeBase"
    if eal.does_asset_exist(path):
        eal.delete_asset(path)
    mat = asset_tools.create_asset("M_EscapeBase", MAT_DIR, unreal.Material, unreal.MaterialFactoryNew())

    color = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -900, -100)
    color.set_editor_property("parameter_name", "Color")
    color.set_editor_property("default_value", unreal.LinearColor(0.5, 0.5, 0.5, 1.0))

    glow = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -900, 150)
    glow.set_editor_property("parameter_name", "Glow")
    glow.set_editor_property("default_value", 0.0)

    hint = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -900, 350)
    hint.set_editor_property("parameter_name", "HintGlow")
    hint.set_editor_property("default_value", 0.0)

    warm = mel.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -900, 500)
    warm.set_editor_property("constant", unreal.LinearColor(1.0, 0.8, 0.35, 1.0))

    # Emissive = Color * Glow + lerp(Color, warm, 0.6) * HintGlow
    tint = mel.create_material_expression(mat, unreal.MaterialExpressionLinearInterpolate, -600, 450)
    tint.set_editor_property("const_alpha", 0.6)
    mel.connect_material_expressions(color, "", tint, "A")
    mel.connect_material_expressions(warm, "", tint, "B")

    hint_mul = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -400, 400)
    mel.connect_material_expressions(tint, "", hint_mul, "A")
    mel.connect_material_expressions(hint, "", hint_mul, "B")

    base_mul = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -400, 150)
    mel.connect_material_expressions(color, "", base_mul, "A")
    mel.connect_material_expressions(glow, "", base_mul, "B")

    emissive = mel.create_material_expression(mat, unreal.MaterialExpressionAdd, -200, 250)
    mel.connect_material_expressions(base_mul, "", emissive, "A")
    mel.connect_material_expressions(hint_mul, "", emissive, "B")

    rough = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -400, 0)
    rough.set_editor_property("r", 0.75)

    mel.connect_material_property(color, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(emissive, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    mat.set_editor_property("used_with_instanced_static_meshes", True)
    mel.recompile_material(mat)
    eal.save_loaded_asset(mat)
    return mat


def build_instance(name, parent, color, glow=0.0):
    path = MAT_DIR + "/" + name
    mic = eal.load_asset(path) if eal.does_asset_exist(path) else asset_tools.create_asset(
        name, MAT_DIR, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mic.set_editor_property("parent", parent)
    mel.set_material_instance_vector_parameter_value(mic, "Color", unreal.LinearColor(*color, 1.0))
    mel.set_material_instance_scalar_parameter_value(mic, "Glow", glow)
    eal.save_loaded_asset(mic)
    return mic


# ------------------------------------------------------------------------------------------------
# Blueprints

def build_blueprint(name, cpp_class):
    path = BP_DIR + "/" + name
    if not eal.does_asset_exist(path):
        factory = unreal.BlueprintFactory()
        factory.set_editor_property("parent_class", unreal.load_class(None, "/Script/SessionAnalyticsDemo." + cpp_class))
        bp = asset_tools.create_asset(name, BP_DIR, unreal.Blueprint, factory)
        unreal.BlueprintEditorLibrary.compile_blueprint(bp)
        eal.save_loaded_asset(bp)
    return eal.load_blueprint_class(path)


# ------------------------------------------------------------------------------------------------
# Level helpers

actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
cube = eal.load_asset("/Engine/BasicShapes/Cube")


def spawn(cls, loc, yaw=0.0, label=None):
    actor = actors.spawn_actor_from_class(cls, unreal.Vector(*loc), unreal.Rotator(0.0, 0.0, yaw))
    if label:
        actor.set_actor_label(label)
    return actor


def block(label, center, size, material, folder="Room"):
    """Gray-box cube: center and size in cm."""
    actor = spawn(unreal.StaticMeshActor, center, label=label)
    comp = actor.static_mesh_component
    comp.set_static_mesh(cube)
    comp.set_material(0, material)
    actor.set_actor_scale3d(unreal.Vector(size[0] / 100.0, size[1] / 100.0, size[2] / 100.0))
    actor.set_folder_path(folder)
    return actor


def set_props(actor, **props):
    for key, value in props.items():
        actor.set_editor_property(key, value)


def set_tags(actor, *tags):
    actor.set_editor_property("tags", [unreal.Name(t) for t in tags])


def rect_light(label, loc, target, intensity, tags, color=(1.0, 0.95, 0.85), width=120.0, height=60.0):
    rot = unreal.MathLibrary.find_look_at_rotation(unreal.Vector(*loc), unreal.Vector(*target))
    actor = actors.spawn_actor_from_class(unreal.RectLight, unreal.Vector(*loc), rot)
    actor.set_actor_label(label)
    comp = actor.get_component_by_class(unreal.RectLightComponent)
    comp.set_mobility(unreal.ComponentMobility.MOVABLE)
    comp.set_editor_property("intensity_units", unreal.LightUnits.CANDELAS)
    comp.set_editor_property("intensity", intensity)
    comp.set_light_color(unreal.LinearColor(color[0], color[1], color[2], 1.0))
    comp.set_editor_property("source_width", width)
    comp.set_editor_property("source_height", height)
    comp.set_editor_property("attenuation_radius", 700.0)
    set_tags(actor, *tags)
    actor.set_folder_path("Lights")
    return actor


# ------------------------------------------------------------------------------------------------
# Build

base = build_base_material()
mi_floor = build_instance("MI_Floor", base, (0.22, 0.2, 0.18))
mi_wall = build_instance("MI_Wall", base, (0.52, 0.5, 0.47))
mi_ceiling = build_instance("MI_Ceiling", base, (0.75, 0.75, 0.75))
mi_stool = build_instance("MI_Stool", base, (0.3, 0.22, 0.15))
mi_exit = build_instance("MI_ExitGlow", base, (1.0, 0.98, 0.92), glow=3.0)
log("materials ok")

bp = {name: build_blueprint(name, cls) for name, cls in [
    ("BP_EscapeGameMode", "EscapeGameMode"),
    ("BP_EscapeCharacter", "EscapeCharacter"),
    ("BP_EscapeRoomManager", "EscapeRoomManager"),
    ("BP_Inspectable", "EscapeInteractable"),
    ("BP_Frame", "EscapeFrame"),
    ("BP_Desk", "EscapeDesk"),
    ("BP_Paint", "EscapePaint"),
    ("BP_Easel", "EscapeEasel"),
    ("BP_Safe", "EscapeSafe"),
    ("BP_Door", "EscapeDoor"),
]}
gm_cdo = unreal.get_default_object(bp["BP_EscapeGameMode"])
gm_cdo.set_editor_property("default_pawn_class", bp["BP_EscapeCharacter"])
eal.save_asset(BP_DIR + "/BP_EscapeGameMode")
log("blueprints ok")

if eal.does_asset_exist(LEVEL):
    eal.delete_asset(LEVEL)
if not unreal.EditorLoadingAndSavingUtils.new_blank_map(False):
    raise RuntimeError("new_blank_map failed")

# Room: 600 x 600 x 300, centered on the origin, walls 20 thick outside the play space. Door opening on +X.
block("Floor", (0, 0, -10), (640, 640, 20), mi_floor)
block("Ceiling", (0, 0, 310), (640, 640, 20), mi_ceiling)
block("Wall_-X", (-310, 0, 150), (20, 640, 300), mi_wall)
block("Wall_+Y", (0, 310, 150), (640, 20, 300), mi_wall)
block("Wall_-Y", (0, -310, 150), (640, 20, 300), mi_wall)
block("Wall_+X_L", (310, -182.5, 150), (20, 255, 300), mi_wall)
block("Wall_+X_R", (310, 182.5, 150), (20, 255, 300), mi_wall)
block("Wall_+X_Top", (310, 0, 260), (20, 110, 80), mi_wall)
block("Exit_Floor", (420, 0, -10), (200, 200, 20), mi_floor, folder="Exit")
block("Exit_Glow", (480, 0, 150), (20, 300, 320), mi_exit, folder="Exit")
block("PaintStool", (-110, -262, 30), (80, 36, 60), mi_stool, folder="Props")

start = spawn(unreal.PlayerStart, (-220, 0, 100), 0.0, "PlayerStart")

manager = spawn(bp["BP_EscapeRoomManager"], (0, 0, 0), 0.0, "EscapeRoomManager")

desk = spawn(bp["BP_Desk"], (0, -250, 0), 90.0, "Desk")
memo = spawn(bp["BP_Inspectable"], (-30, -252, 75.3), 100.0, "Memo")
set_props(memo, object_id="memo", display_name="메모", puzzle_ids=[1],
          inspect_message="메모: \"열쇠는 늘 마지막 작품 뒤에\"", base_color=unreal.LinearColor(0.95, 0.93, 0.85, 1.0))
memo.set_actor_scale3d(unreal.Vector(0.15, 0.21, 0.005))
set_tags(memo, "P1H1")

# Frames on the +Y wall, facing -Y. Code = counts from smallest to largest: A(3) B(7) C(1) D(5) -> 3715.
# Latest date is C (2024.02): the drawer key is behind it.
frames = [
    # id, center x, (w, h), count, date, canvas, objects
    ("frame_c", -48.0, (60, 75), 1, "2024.02", (0.2, 0.42, 0.25), (0.95, 0.95, 0.9)),
    ("frame_a", 16.0, (32, 40), 3, "2021.11", (0.88, 0.84, 0.72), (0.75, 0.1, 0.08)),
    ("frame_d", 92.5, (85, 100), 5, "2022.07", (0.35, 0.42, 0.55), (0.95, 0.5, 0.1)),
    ("frame_b", 176.0, (45, 55), 7, "2019.04", (0.08, 0.1, 0.3), (0.95, 0.8, 0.15)),
]
for fid, x, size, count, date, canvas, objs in frames:
    frame = spawn(bp["BP_Frame"], (x, 299, 165), -90.0, fid)
    set_props(frame, object_id=fid, frame_size=unreal.Vector2D(*size), object_count=count, date_label=date,
              canvas_color=unreal.LinearColor(*canvas, 1.0), object_color=unreal.LinearColor(*objs, 1.0))

easel = spawn(bp["BP_Easel"], (-215, -215, 0), 45.0, "Easel")

paints = [("red", "빨강", (0.75, 0.06, 0.05), -135.0), ("blue", "파랑", (0.04, 0.12, 0.75), -110.0),
          ("yellow", "노랑", (0.95, 0.75, 0.05), -85.0)]
for pid, name, color, x in paints:
    paint = spawn(bp["BP_Paint"], (x, -258, 60), 90.0, "Paint_" + pid)
    set_props(paint, object_id="paint_" + pid, paint_id=pid, color_name=name, display_name=name + " 물감",
              paint_color=unreal.LinearColor(*color, 1.0))
    if pid in ("blue", "yellow"):
        set_tags(paint, "P2H2")

safe = spawn(bp["BP_Safe"], (-200, 220, 0), -90.0, "Safe")
cover = spawn(bp["BP_Inspectable"], (-200, 218, 50), 0.0, "SafeCover")
set_props(cover, object_id="cover", display_name="천 덮개", inspect_message="무거운 천이 덮여 있다. 단단히 고정돼 있어 들출 수 없다.",
          base_color=unreal.LinearColor(0.38, 0.1, 0.1, 1.0))
cover.set_actor_scale3d(unreal.Vector(0.96, 0.86, 1.0))
set_tags(cover, "SafeCover")
set_props(manager, safe_cover=cover)

door = spawn(bp["BP_Door"], (300, 0, 0), 180.0, "Door")

# Lights: one dim ambient, plus per-puzzle area lights (only the current puzzle's are bright).
ambient = actors.spawn_actor_from_class(unreal.PointLight, unreal.Vector(0, 0, 270), unreal.Rotator(0, 0, 0))
ambient.set_actor_label("Light_Ambient")
amb = ambient.get_component_by_class(unreal.PointLightComponent)
amb.set_mobility(unreal.ComponentMobility.MOVABLE)
amb.set_editor_property("intensity_units", unreal.LightUnits.CANDELAS)
amb.set_editor_property("intensity", 12.0)
amb.set_editor_property("attenuation_radius", 1000.0)
amb.set_editor_property("source_radius", 40.0)
ambient.set_folder_path("Lights")

rect_light("Light_P1_Frames", (60, 150, 285), (60, 300, 160), 60.0, ["Light_P1"])
rect_light("Light_P1_Desk", (0, -120, 285), (0, -250, 75), 45.0, ["Light_P1"])
rect_light("Light_P2_Easel", (-120, -120, 285), (-150, -250, 70), 50.0, ["Light_P2"], color=(1.0, 0.9, 0.8))
rect_light("Light_P2_Lamp", (180, 0, 285), (300, 0, 240), 25.0, ["Light_P2"], color=(0.8, 1.0, 0.85))
rect_light("Light_P3_Safe", (-120, 100, 285), (-200, 220, 40), 50.0, ["Light_P3"], color=(1.0, 0.85, 0.65))
rect_light("Light_P3_Frames", (60, 150, 285), (60, 300, 160), 60.0, ["Light_P3"], color=(1.0, 0.85, 0.65))
# The exit light sits outside the door wall. Its radius must not reach into the room: a light that only gets blocked
# by the wall's shadow leaks through as shadow artifacts on the ceiling (seen in PIE).
exit_light = actors.spawn_actor_from_class(unreal.PointLight, unreal.Vector(460, 0, 150), unreal.Rotator(0, 0, 0))
exit_light.set_actor_label("Light_Exit")
el = exit_light.get_component_by_class(unreal.PointLightComponent)
el.set_editor_property("intensity_units", unreal.LightUnits.CANDELAS)
el.set_editor_property("intensity", 40.0)
el.set_editor_property("attenuation_radius", 140.0)
el.set_editor_property("cast_shadows", False)
exit_light.set_folder_path("Lights")

# Fixed exposure so the per-puzzle light changes are visible instead of being auto-exposed away.
ppv = spawn(unreal.PostProcessVolume, (0, 0, 150), 0.0, "PostProcess")
ppv.set_editor_property("unbound", True)
settings = ppv.get_editor_property("settings")
EXPOSURE_EV100 = 3.6
settings.set_editor_property("override_auto_exposure_min_brightness", True)
settings.set_editor_property("auto_exposure_min_brightness", EXPOSURE_EV100)
settings.set_editor_property("override_auto_exposure_max_brightness", True)
settings.set_editor_property("auto_exposure_max_brightness", EXPOSURE_EV100)
ppv.set_editor_property("settings", settings)

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
world_settings = world.get_world_settings()
world_settings.set_editor_property("default_game_mode", bp["BP_EscapeGameMode"])
world_settings.set_editor_property("force_no_precomputed_lighting", True)

if not unreal.EditorLoadingAndSavingUtils.save_map(world, LEVEL):
    raise RuntimeError("save_map failed")
log("level saved " + LEVEL)
