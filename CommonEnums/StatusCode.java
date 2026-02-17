package CommonEnums;

public enum StatusCode {
    STATUS_OK(0),
    STATUS_ERROR(1),
    STATUS_WARNING(2);

    private final int value;

    StatusCode(int value) {
        this.value = value;
    }

    public int getValue() {
        return value;
    }
}
