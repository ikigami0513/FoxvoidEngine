# This file is strictly for Visual Studio Code (or other IDEs) to provide type hinting 
# and autocompletion. The "..." (Ellipsis) indicate to Python that the actual 
# implementation is handled natively in C++.

from typing import TypeVar, Type, Optional, List

# Type variable for smart autocompletion in the get_component method.
# It ensures that getting a Transform2d actually returns a Transform2d type in the IDE.
T = TypeVar('T', bound='Component')


def log(msg: str) -> None:
    """
    Prints a message directly to the C++ engine console.
    Bypasses Python's standard stdout to prevent crashes with graphics libraries.
    """
    ...


def set_pixel_art_mode(enable: bool) -> None:
    """
    Globally configures texture filtering. 
    Call this BEFORE instantiating GameObjects with sprites.
    - True: Nearest-neighbor filtering (Sharp, blocky, perfect for pixel art).
    - False: Bilinear filtering (Smooth, interpolated edges).
    """
    ...


def is_pixel_art_mode() -> bool:
    """Returns whether the engine is currently in pixel art mode."""
    ...


class GameObject:
    """Represents an entity within the engine's scene."""
    @staticmethod
    def instantiate(name: str) -> GameObject:
        """
        Creates a new empty GameObject in the active scene.
        
        Args:
            name: The name of the new entity.
            
        Returns:
            The newly created GameObject instance.
        """
        ...

    def destroy(self) -> None:
        """
        Marks this GameObject for destruction. 
        It will be safely removed from memory at the end of the current C++ frame.
        """
        ...
    
    def get_component(self, type_obj: Type[T]) -> Optional[T]:
        """
        Retrieves a component attached to this GameObject.
        
        Args:
            type_obj: The class type of the component to retrieve (e.g., Transform2d).
            
        Returns:
            The component instance if found, otherwise None.
        """
        ...

    def add_component(self, type_obj: Type[T], *args) -> T:
        """
        Dynamically instantiates and attaches a new component to this GameObject.
        
        Args:
            type_obj: The class type of the component to create.
            *args: Variable arguments passed directly to the C++ constructor of the component.
            
        Returns:
            The newly created and attached component instance.
        """
        ...


class Vector2:
    """A 2D vector representing mathematical points or directions."""
    x: float
    y: float
    def __init__(self, x: float = 0.0, y: float = 0.0) -> None: ...


class Component:
    """Base class for all engine scripts and components."""
    
    def __init__(self) -> None: 
        """Initializes the C++ base component memory."""
        ...
    
    @property
    def game_object(self) -> GameObject:
        """The owning GameObject of this component (Read-only)."""
        ...


class Transform2d(Component):
    """Component managing spatial position, rotation, and scale."""
    
    position: Vector2
    scale: Vector2
    rotation: float
    
    def __init__(self, x: float = 0.0, y: float = 0.0) -> None: 
        """Creates a new Transform2d component."""
        ...
    

class Keys:
    """Raylib keyboard key codes."""
    KEY_NULL: int
    KEY_APOSTROPHE: int
    KEY_COMMA: int
    KEY_MINUS: int
    KEY_PERIOD: int
    KEY_SLASH: int

    KEY_ZERO: int
    KEY_ONE: int
    KEY_TWO: int
    KEY_THREE: int
    KEY_FOUR: int
    KEY_FIVE: int
    KEY_SIX: int
    KEY_SEVEN: int
    KEY_EIGHT: int
    KEY_NINE: int

    KEY_SEMICOLON: int
    KEY_EQUAL: int

    KEY_A: int
    KEY_B: int
    KEY_C: int
    KEY_D: int
    KEY_E: int
    KEY_F: int
    KEY_G: int
    KEY_H: int
    KEY_I: int
    KEY_J: int
    KEY_K: int
    KEY_L: int
    KEY_M: int
    KEY_N: int
    KEY_O: int
    KEY_P: int
    KEY_Q: int
    KEY_R: int
    KEY_S: int
    KEY_T: int
    KEY_U: int
    KEY_V: int
    KEY_W: int
    KEY_X: int
    KEY_Y: int
    KEY_Z: int

    KEY_LEFT_BRACKET: int
    KEY_BACKSLASH: int
    KEY_RIGHT_BRACKET: int
    KEY_GRAVE: int

    KEY_SPACE: int
    KEY_ESCAPE: int
    KEY_ENTER: int
    KEY_TAB: int
    KEY_BACKSPACE: int
    KEY_INSERT: int
    KEY_DELETE: int
    RIGHT: int
    LEFT: int
    DOWN: int
    UP: int
    KEY_PAGE_UP: int
    KEY_PAGE_DOWN: int
    KEY_HOME: int
    KEY_END: int
    KEY_CAPS_LOCK: int
    KEY_SCROLL_LOCK: int
    KEY_NUM_LOCK: int
    KEY_PRINT_SCREEN: int
    KEY_PAUSE: int

    KEY_F1: int
    KEY_F2: int
    KEY_F3: int
    KEY_F4: int
    KEY_F5: int
    KEY_F6: int
    KEY_F7: int
    KEY_F8: int
    KEY_F9: int
    KEY_F10: int
    KEY_F11: int
    KEY_F12: int

    KEY_LEFT_SHIFT: int
    KEY_LEFT_CONTROL: int
    KEY_LEFT_ALT: int
    KEY_LEFT_SUPER: int
    KEY_RIGHT_SHIFT: int
    KEY_RIGHT_CONTROL: int
    KEY_RIGHT_ALT: int
    KEY_RIGHT_SUPER: int
    KEY_KB_MENU: int

    KEY_KP_0: int
    KEY_KP_1: int
    KEY_KP_2: int
    KEY_KP_3: int
    KEY_KP_4: int
    KEY_KP_5: int
    KEY_KP_6: int
    KEY_KP_7: int
    KEY_KP_8: int
    KEY_KP_9: int

    KEY_KP_DECIMAL: int
    KEY_KP_DIVIDE: int
    KEY_KP_MULTIPLY: int
    KEY_KP_SUBTRACT: int
    KEY_KP_ADD: int
    KEY_KP_ENTER: int
    KEY_KP_EQUAL: int

    KEY_BACK: int
    KEY_MENU: int
    KEY_VOLUME_UP: int
    KEY_VOLUME_DOWN: int


class Input:
    """Handles keyboard and mouse inputs from the engine."""
    
    @staticmethod
    def is_key_down(key: int) -> bool:
        """
        Checks if a key is currently being held down.
        Returns True every frame the key remains pressed.
        """
        ...
        
    @staticmethod
    def is_key_pressed(key: int) -> bool:
        """
        Checks if a key has been pressed once.
        Returns True only on the exact frame the key was pushed down.
        """
        ...


class SpriteRenderer(Component):
    """Renders a 2D texture at the position defined by the entity's Transform2d."""
    
    def __init__(self, texture_path: str) -> None: 
        """
        Loads an image file into GPU memory for rendering.
        
        Args:
            texture_path: The relative path to the image file (e.g., 'assets/player.png').
        """
        ...


class SpriteSheetRenderer(Component):
    """Renders a specific frame from a grid-based spritesheet texture."""
    
    def __init__(self, texture_path: str, columns: int, rows: int) -> None: 
        """
        Loads a spritesheet into GPU memory.
        
        Args:
            texture_path: The relative path to the image file.
            columns: The number of columns in the spritesheet grid.
            rows: The number of rows in the spritesheet grid.
        """
        ...
        
    @property
    def frame(self) -> int:
        """The current frame index being rendered (0 to frame_count - 1)."""
        ...
        
    @frame.setter
    def frame(self, value: int) -> None: ...
    
    @property
    def frame_count(self) -> int:
        """Total number of frames in the spritesheet (Read-only)."""
        ...


class Animation2d(Component):
    """
    Handles frame-by-frame animation by controlling a SpriteSheetRenderer.
    Performance is maximized as the timer logic runs natively in C++.
    """
    
    def __init__(self, frames: List[int], speed: float, loop: bool = True) -> None: 
        """
        Initializes the animation sequence.
        
        Args:
            frames: Sequence of frame indices to play (e.g., [0, 1, 2, 3]).
            speed: Time in seconds each frame is displayed.
            loop: If True, the animation repeats indefinitely.
        """
        ...
