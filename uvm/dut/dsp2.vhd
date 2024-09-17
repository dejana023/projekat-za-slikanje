library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;

-- Operation : temp_1 + temp_2 or temp_1 - temp_2 based on ADD_SUB

entity dsp2 is
    generic (
          FIXED_SIZE : integer := 48
          );
    port (
          clk : in std_logic;
          rst : in std_logic;
          u1_i : in std_logic_vector(FIXED_SIZE - 1 downto 0); -- i_cose * (i - iradius), temp_1
          u2_i : in std_logic_vector(FIXED_SIZE - 1 downto 0); -- i_sine * (j - iradius), temp_2
          ADD_SUB : in std_logic; -- '0' for add, '1' for subtract
          res_o : out std_logic_vector(FIXED_SIZE - 1 downto 0) -- temp_3
          );
end dsp2;

architecture Behavioral of dsp2 is
    attribute use_dsp : string;
    attribute use_dsp of Behavioral : architecture is "yes";
    signal u1_reg : signed(FIXED_SIZE - 1 downto 0);
    signal u2_reg : signed(FIXED_SIZE - 1 downto 0);    
    signal res_reg : signed(FIXED_SIZE - 1 downto 0);
begin
    process(clk)
    begin
        if (rising_edge(clk)) then
            if (rst = '1') then
                -- Reset all registers to zero
                u1_reg <= (others => '0');
                u2_reg <= (others => '0');
                res_reg <= (others => '0');
            else
                -- Register inputs
                u1_reg <= signed(u1_i);
                u2_reg <= signed(u2_i);
                
                -- Perform add or subtract based on ADD_SUB
                if (ADD_SUB = '0') then
                    res_reg <= u1_reg + u2_reg;
                else
                    res_reg <= u1_reg - u2_reg;
                end if;  
            end if;
        end if;
    end process;

    -- Convert result to std_logic_vector for output
    res_o <= std_logic_vector(res_reg);
end Behavioral;
