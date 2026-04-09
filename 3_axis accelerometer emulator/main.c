#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

/* ================== DEFINES ================== */

#define DEVICE_ADDR        0x6A   /**< I2C device address */
#define REGISTER_COUNT     256    /**< Total number of registers */

/* Device Identification */
#define WHO_AM_I_REG       0x0F   /**< Device ID register */

/* Control Registers */
#define CTRL1_XL           0x10   /**< Accelerometer control register */

/* Accelerometer Output Registers */
#define OUTX_L_A           0x28   /**< X-axis LSB */
#define OUTX_H_A           0x29   /**< X-axis MSB */

#define OUTY_L_A           0x2A   /**< Y-axis LSB */
#define OUTY_H_A           0x2B   /**< Y-axis MSB */

#define OUTZ_L_A           0x2C   /**< Z-axis LSB */
#define OUTZ_H_A           0x2D   /**< Z-axis MSB */
/* ================== I2C STATE ================== */

typedef enum {
    I2C_IDLE,
    I2C_START,
    I2C_ADDRESS,
    I2C_REGISTER,
    I2C_READ,
    I2C_WRITE
} i2c_state_t;

i2c_state_t state = I2C_IDLE;

/* ================== SENSOR STATE ================== */

typedef struct {
    uint8_t accel_enabled;  /**< Flag indicating if accelerometer is enabled */
    float   angle;          /**< Current angle used for motion simulation */
    int     bias;           /**< Constant offset applied to sensor output */
} sensor_state_t;

/* Global instance of sensor state */
sensor_state_t sensor;

/* ================== MEMORY ================== */

uint8_t registers[REGISTER_COUNT];
uint8_t current_reg = 0;

/* ================== EMULATOR INIT ================== */

/**
 * @brief Initialize emulator state and register map
 *
 * Performs the following initialization:
 *  - Clears all device registers
 *  - Sets fixed device identification register (WHO_AM_I)
 *  - Initializes internal sensor parameters
 *
 * Sensor initial state:
 *  - Accelerometer disabled
 *  - Angle reset to zero
 *  - Bias initialized to a fixed offset
 */
void emulator_init()
{
    /* Clear entire register map */
    for (int i = 0; i < REGISTER_COUNT; i++)
        registers[i] = 0;

    /* Set device ID (used for identification check) */
    registers[WHO_AM_I_REG] = 0x6A;

    /* Initialize sensor internal state */
    sensor.accel_enabled = 0;  /* Disabled by default */
    sensor.angle = 0;          /* Initial motion angle */
    sensor.bias = 100;         /* Constant offset (simulated bias) */
}

/* ================== SENSOR UPDATE ================== */

/**
 * @brief Update simulated accelerometer data
 *
 * Generates synthetic accelerometer values and updates the
 * corresponding output registers.
 *
 * Features simulated:
 *  - Sinusoidal motion (X and Y axes)
 *  - Constant acceleration (Z axis)
 *  - Random noise
 *  - Sensor bias
 *
 * Data is written in little-endian format:
 *   LSB → lower register
 *   MSB → higher register
 *
 * The update occurs only when the accelerometer is enabled.
 */
void emulator_update()
{
    /* Skip update if accelerometer is disabled */
    if (!sensor.accel_enabled)
        return;

    /* Simulate motion using angular progression */
    sensor.angle += 0.1;

    /* Generate ideal motion values */
    float x_motion = 1000 * sin(sensor.angle);
    float y_motion = 1000 * cos(sensor.angle);
    float z_motion = 1000;  /* Constant gravity-like component */

    /* Add random noise (range: -25 to +24) */
    int noise_x = (rand() % 50) - 25;
    int noise_y = (rand() % 50) - 25;
    int noise_z = (rand() % 50) - 25;

    /* Combine motion, noise, and bias */
    int16_t x = (int16_t)(x_motion + noise_x + sensor.bias);
    int16_t y = (int16_t)(y_motion + noise_y + sensor.bias);
    int16_t z = (int16_t)(z_motion + noise_z + sensor.bias);

    /* Store data in register map (Little Endian format) */
    registers[OUTX_L_A] = x & 0xFF;
    registers[OUTX_H_A] = (x >> 8) & 0xFF;

    registers[OUTY_L_A] = y & 0xFF;
    registers[OUTY_H_A] = (y >> 8) & 0xFF;

    registers[OUTZ_L_A] = z & 0xFF;
    registers[OUTZ_H_A] = (z >> 8) & 0xFF;
}

/* ================== I2C FUNCTIONS ================== */

/**
 * @brief Generate I2C START condition
 *
 * Initiates a new I2C transaction by transitioning the state
 * machine to I2C_START. This indicates the beginning of communication
 * with a slave device.
 *
 * After this, the next expected operation is sending the device address.
 *
 * State transition:
 *   I2C_IDLE → I2C_START
 */
void i2c_start()
{
    /* Set state to START condition */
    state = I2C_START;

    /* Debug log: transaction start */
    printf("[I2C] START\n");
}

/**
 * @brief Send device address over I2C bus
 *
 * Verifies whether the provided address matches the device address.
 * If matched, transitions the state machine based on the operation:
 *   - Read operation  → I2C_READ
 *   - Write operation → I2C_REGISTER (next phase: register selection)
 *
 * If the address does not match, the transaction is terminated and
 * the state is reset to I2C_IDLE.
 *
 * This function is valid only when the I2C state machine is in
 * I2C_START state.
 *
 * @param[in] addr 7-bit device address
 * @param[in] read Read/Write flag (1 = Read, 0 = Write)
 */
void i2c_send_address(uint8_t addr, uint8_t read)
{
    /* Validate state: must be called after START condition */
    if (state != I2C_START)
        return;

    /* Check if the address matches the device */
    if (addr == DEVICE_ADDR)
    {
        /* Transition based on operation type */
        state = read ? I2C_READ : I2C_REGISTER;

        /* Debug log: address acknowledged */
        printf("[I2C] ADDRESS OK\n");
    }
    else
    {
        /* Debug log: address mismatch */
        printf("[I2C] ADDRESS FAIL\n");

        /* Reset state machine on failure */
        state = I2C_IDLE;
    }
}

/**
 * @brief Send register address to the I2C device
 *
 * Sets the internal register pointer to the specified address.
 * After receiving the register address, the state machine transitions
 * to I2C_WRITE state for the data phase.
 *
 * This function is valid only when the I2C state machine is in
 * I2C_REGISTER state.
 *
 * @param[in] reg Register address to access
 */
void i2c_send_register(uint8_t reg)
{
    /* Validate state: register phase expected */
    if (state != I2C_REGISTER)
        return;

    /* Set current register pointer */
    current_reg = reg;

    /* Transition to write state for data transfer */
    state = I2C_WRITE;

    /* Debug log: register selection */
    printf("[I2C] REG = 0x%X\n", reg);
}

/**
 * @brief Write a byte of data to the current register
 *
 * Writes the provided data to the register pointed by current_reg
 * and increments the register pointer to support sequential writes.
 *
 * If the written register corresponds to CTRL1_XL, the emulator updates
 * the internal sensor state to enable or disable the accelerometer.
 *
 * This function is valid only when the I2C state machine is in
 * I2C_WRITE state.
 *
 * @param[in] data Data byte to be written into the register
 */
void i2c_write_data(uint8_t data)
{
    /* Validate state: write operation allowed only in I2C_WRITE */
    if (state != I2C_WRITE)
        return;

    /* Write data to current register */
    registers[current_reg] = data;

    /* Handle control register side effects */
    if (current_reg == CTRL1_XL)
    {
        /* Enable/disable accelerometer based on configuration */
        sensor.accel_enabled = (data != 0);

        /* Debug log for state change */
        printf("[EMULATOR] Accel Enabled = %d\n", sensor.accel_enabled);
    }

    /* Auto-increment register address for sequential writes */
    current_reg++;
}

/**
 * @brief Read a byte of data from the current register
 *
 * Returns the data at the current register address and increments
 * the register pointer to support sequential (burst) reads.
 *
 * This function is valid only when the I2C state machine is in
 * I2C_READ state.
 *
 * @return uint8_t Data read from register, or 0 if state is invalid
 */
uint8_t i2c_read_data()
{
    /* Validate state: read operation allowed only in I2C_READ */
    if (state != I2C_READ)
        return 0;

    /* Read data from current register */
    uint8_t data = registers[current_reg];

    /* Auto-increment register address for next read */
    current_reg++;

    return data;
}

/**
 * @brief Generate I2C STOP condition
 *
 * Terminates the current I2C transaction and returns the bus
 * to the idle state. No further communication is allowed until
 * a new START condition is issued.
 *
 * State transition:
 *   Any Active State → I2C_IDLE
 */
void i2c_stop()
{
    /* Set I2C state to idle */
    state = I2C_IDLE;

    /* Log STOP condition (for debugging/visualization) */
    printf("[I2C] STOP\n");
}

/* ================== DRIVER SIMULATION ================== */

/**
 * @brief Initialize accelerometer via I2C
 *
 * Configures the sensor by writing to the CTRL1_XL register.
 * This enables the accelerometer and sets its operating mode.
 *
 * I2C Transaction sequence:
 *   START → DEVICE ADDRESS (WRITE) → REGISTER ADDRESS → DATA → STOP
 *
 * Register Configuration:
 *   CTRL1_XL (0x10) = 0x60
 *   - Enables accelerometer
 *   - Sets output data rate and scale (as per device configuration)
 */
void driver_init()
{
    /* Start I2C write transaction */
    i2c_start();

    /* Send device address with write operation (0) */
    i2c_send_address(DEVICE_ADDR, 0);

    /* Select control register for accelerometer configuration */
    i2c_send_register(CTRL1_XL);

    /* Write configuration value to enable accelerometer */
    i2c_write_data(0x60);

    /* End I2C transaction */
    i2c_stop();
}

/**
 * @brief Read accelerometer X, Y, Z axis data
 *
 * Initiates an I2C read transaction to fetch 6 consecutive bytes
 * starting from the OUTX_L_A register. Each axis value is composed
 * of two bytes (LSB and MSB), which are combined to form a 16-bit
 * signed integer.
 *
 * Data format:
 *   X = [OUTX_H_A | OUTX_L_A]
 *   Y = [OUTY_H_A | OUTY_L_A]
 *   Z = [OUTZ_H_A | OUTZ_L_A]
 *
 * @param[out] x Pointer to store X-axis data
 * @param[out] y Pointer to store Y-axis data
 * @param[out] z Pointer to store Z-axis data
 */
void driver_read_xyz(int16_t *x, int16_t *y, int16_t *z)
{
    /* Start I2C read transaction */
    i2c_start();
    i2c_send_address(DEVICE_ADDR, 1);

    /* Set starting register address (auto-increment assumed) */
    current_reg = OUTX_L_A;

    /* Read X-axis data (LSB first, then MSB) */
    uint8_t xl = i2c_read_data();
    uint8_t xh = i2c_read_data();

    /* Read Y-axis data */
    uint8_t yl = i2c_read_data();
    uint8_t yh = i2c_read_data();

    /* Read Z-axis data */
    uint8_t zl = i2c_read_data();
    uint8_t zh = i2c_read_data();

    /* End I2C transaction */
    i2c_stop();

    /* Combine MSB and LSB to form signed 16-bit values */
    *x = (int16_t)((xh << 8) | xl);
    *y = (int16_t)((yh << 8) | yl);
    *z = (int16_t)((zh << 8) | zl);
}

/* ================== MAIN ================== */

/**
 * @brief Main entry point of the emulator application
 *
 * Initializes the sensor emulator and driver, then continuously
 * simulates accelerometer data and reads it via I2C interface.
 *
 * This loop mimics real-time sensor data acquisition.
 */
int main()
{
    /* Initialize random generator (used for sensor noise simulation) */
    srand(time(NULL));

    /* Setup emulator:
     * - Reset registers
     * - Initialize sensor state
     */
    emulator_init();

    printf("Starting Emulator...\n");

    /* Configure sensor via driver:
     * - Enable accelerometer (CTRL1_XL)
     */
    driver_init();

    /* Main loop: simulate + read sensor continuously */
    while (1)
    {
        /* Update internal sensor values:
         * - Simulated motion (sin/cos)
         * - Noise + bias added
         */
        emulator_update();

        /* Read accelerometer data (X, Y, Z) */
        int16_t x, y, z;
        driver_read_xyz(&x, &y, &z);

        /* Display output */
        printf("X=%d | Y=%d | Z=%d\n", x, y, z);
        printf("-------------------------\n");

        /* Simple delay (non-precise, for visualization only) */
        for (volatile long i = 0; i < 100000000; i++);
    }

    return 0;
}
