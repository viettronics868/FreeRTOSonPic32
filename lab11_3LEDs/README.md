# Standard Operating Procedure (SOP)

# Title: FreeRTOS Practice on Curiosity 2.0 PIC32MZ EF Development Board

# 1. Purpose

    This SOP provides step-by-step instructions to set up and build a FreeRTOS project using MPLAB X IDE and MPLAB Code Configurator (MCC) on the Curiosity 2.0 PIC32MZ EF Development Board.

# 2. Scope

    This procedure applies to students, developers, or engineers practicing FreeRTOS on PIC32MZ EF devices using Harmony 3 framework, UART peripherals, and DMA modules.

# 3. Modules / Technology Used

    Peripheral Modules: UART, DMA, FreeRTOS

# 4. Hardware Used

    Curiosity PIC32MZ EF 2.0 Development Board

# 5. Software / Tools Used

    MPLAB X IDE v6.25

    MPLAB Code Configurator (MCC) v5.6.2

    XC32 Compiler v4.60

# 6. Setup

    Connect the Curiosity PIC32MZ EF 2.0 Development Board to the host PC.

    Use a Type-A male to micro-B USB cable.

    Connect the cable to the Micro-B Debug USB port (located near the ATSAME70N chip).

# 7. Procedure
Step 1: Create a New Project

    Open MPLAB X IDE.

    Close all existing projects, if any.

    Navigate to File → New Project….

    In the New Project window:

        Categories: Microchip Embedded

        Projects: Application Project(s) → Click Next.

        Device: Select pic32mz2048efm144.

        Tool: Select Curiosity PIC32MZ EF 2.0 (S/N displayed) → Click Next.

        Compiler: XC32 (v4.60) → Click Next.

        Project Name and Location:

            Example: Project Name: lab5-struct

            Example: Project Location: ../Documents/Lab5-FreeRTOS-struct

            Uncheck Set as main project.

        Click Finish.

    Wait for MCC to automatically create the project directories.

Step 2: Configure MCC

    Open MPLAB Code Configurator v5 via:

        Window → MPLAB Code Configurator v5 → Open/Close

        Wait for MCC to load.

    In MCC, open Device Resource → Content Manager → Content Libraries:

        Harmony 3 → Chip Support Package (csp) → Click Apply.

        Harmony 3 → Core (bsp, core) → Click Apply.

        Harmony 3 → System Hardware Definitions (shd) → Click Apply.

        FreeRTOS → FreeRTOS-Kernel v11.20 → Click Apply.

    Close Content Manager panel.

Step 3: Add Required Modules

    Go to Device Resource → Libraries → Harmony → Peripherals → UART → UART6 → Click (+) to add.

        UART6 module will appear in Project Graph.

    Go to Device Resource → Third Party Libraries → RTOS → FreeRTOS → Click (+).

        FreeRTOS module will appear in Project Graph.

Step 4: Configure System and RTOS

    Navigate to Project Graph → System → MIPS Configuration → CACHE:

        Enable Use Cache Maintenance.

    Go to FreeRTOS → RTOS Configuration:

        Set Minimal Stack Size = 256.

        Enable Static Memory Allocation.

        Enable uxTaskGetStackHighWaterMark.

    Open Plugins → Pin Configuration → Pin Settings:

        Pin Number 79 → Function → U6TX.

    Go to DMA Configuration:

        Enable DMAC Channel 5.

        Set Trigger → UART6_TX.

Step 5: Generate Project Files

    Go to Device Resource → Generate.

    Wait for MCC to automatically generate and modify project directories.

# 8. Completion

    The project is now ready for building.

    Proceed with development and testing.

# Have fun practicing FreeRTOS on PIC32MZ EF!