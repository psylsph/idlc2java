package Shapes;

public enum ShapeType {
    CIRCLE_TYPE(0),
    RECTANGLE_TYPE(1),
    TRIANGLE_TYPE(2);

    private final int value;

    ShapeType(int value) {
        this.value = value;
    }

    public int getValue() {
        return value;
    }
}
