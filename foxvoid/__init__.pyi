# This file is strictly for Visual Studio Code (or other IDEs) to provide type hinting 
# and autocompletion. The "..." (Ellipsis) indicate to Python that the actual 
# implementation is handled natively in C++.

from typing import TypeVar, Type, Optional

# Type variable for smart autocompletion in the get_component method.
# It ensures that getting a Transform2d actually returns a Transform2d type in the IDE.
T = TypeVar('T', bound='Component')


def log(msg: str) -> None:
    """
    Prints a message directly to the C++ engine console.
    Bypasses Python's standard stdout to prevent crashes with graphics libraries.
    """
    ...


class GameObject:
    """Represents an entity within the engine's scene."""
    
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
    """Component managing spatial position (X, Y)."""
    
    def __init__(self, x: float = 0.0, y: float = 0.0) -> None: 
        """Creates a new Transform2d component with optional starting coordinates."""
        ...
    
    @property
    def x(self) -> float:
        """The X coordinate of the transform."""
        ...
    
    @x.setter
    def x(self, value: float) -> None: ...
    
    @property
    def y(self) -> float:
        """The Y coordinate of the transform."""
        ...
    
    @y.setter
    def y(self, value: float) -> None: ...
    

class Keys:
    """Raylib keyboard key codes."""
    RIGHT: int
    LEFT: int
    DOWN: int
    UP: int
    SPACE: int


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
