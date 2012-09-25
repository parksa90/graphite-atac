#include "instruction.h"
#include "simulator.h"
#include "tile_manager.h"
#include "tile.h"
#include "core_model.h"
#include "branch_predictor.h"

// Instruction

Instruction::StaticInstructionCosts Instruction::m_instruction_costs;

Instruction::Instruction(InstructionType type, UInt64 opcode, OperandList &operands)
   : m_type(type)
   , m_opcode(opcode)
   , m_address(0)
   , m_size(0)
   , m_operands(operands)
{
}

Instruction::Instruction(InstructionType type)
   : m_type(type)
   , m_opcode(0)
   , m_address(0)
   , m_size(0)
{
}

UInt64 Instruction::getCost()
{
   LOG_ASSERT_ERROR(m_type < MAX_INSTRUCTION_COUNT, "Unknown instruction type: %d", m_type);
   return Instruction::m_instruction_costs[m_type]; 
}

bool Instruction::isSimpleMemoryLoad() const
{
   if (m_operands.size() > 2)
      return false;

   bool memory_read = false;
   bool reg_write = false;
   for (unsigned int i = 0; i < m_operands.size(); i++)
   {
      const Operand& o = m_operands[i];
      if ((o.m_type == Operand::MEMORY) && (o.m_direction == Operand::READ))
         memory_read = true;
      if ((o.m_type == Operand::REG) && (o.m_direction == Operand::WRITE))
         reg_write = true;
   }

   switch (m_operands.size())
   {
   case 1:
      return (memory_read);
   case 2:
      return (memory_read && reg_write);
   default:
      return false;
   }
}

void Instruction::initializeStaticInstructionModel()
{
   m_instruction_costs.resize(MAX_INSTRUCTION_COUNT);
   for(unsigned int i = 0; i < MAX_INSTRUCTION_COUNT; i++)
   {
       char key_name [1024];
       snprintf(key_name, 1024, "core/static_instruction_costs/%s", INSTRUCTION_NAMES[i]);
       UInt32 instruction_cost = Sim()->getCfg()->getInt(key_name, 0);
       m_instruction_costs[i] = instruction_cost;
   }
}

// DynamicInstruction

DynamicInstruction::DynamicInstruction(UInt64 cost, InstructionType type)
   : Instruction(type)
   , m_cost(cost)
{
}

DynamicInstruction::~DynamicInstruction()
{
}

UInt64 DynamicInstruction::getCost()
{
   return m_cost;
}

// SyncInstruction

SyncInstruction::SyncInstruction(UInt64 cost)
   : DynamicInstruction(cost, INST_SYNC)
{ }

// SpawnInstruction

SpawnInstruction::SpawnInstruction(UInt64 time)
   : Instruction(INST_SPAWN)
   , m_time(time)
{ }

UInt64 SpawnInstruction::getCost()
{
   CoreModel *perf = Sim()->getTileManager()->getCurrentCore()->getPerformanceModel();
   perf->setCycleCount(m_time);
   throw CoreModel::AbortInstructionException(); // exit out of handleInstruction
}

// BranchInstruction

BranchInstruction::BranchInstruction(UInt64 opcode, OperandList &l)
   : Instruction(INST_BRANCH, opcode, l)
{ }

UInt64 BranchInstruction::getCost()
{
   CoreModel *perf = Sim()->getTileManager()->getCurrentCore()->getPerformanceModel();
   BranchPredictor *bp = perf->getBranchPredictor();

   DynamicInstructionInfo &i = perf->getDynamicInstructionInfo();
   LOG_ASSERT_ERROR(i.type == DynamicInstructionInfo::BRANCH, "type(%u)", i.type);

   // branch prediction not modeled
   if (bp == NULL)
   {
      perf->popDynamicInstructionInfo();
      return 1;
   }

   bool prediction = bp->predict(getAddress(), i.branch_info.target);
   bool correct = (prediction == i.branch_info.taken);

   bp->update(prediction, i.branch_info.taken, getAddress(), i.branch_info.target);
   UInt64 cost = correct ? 1 : bp->getMispredictPenalty();
      
   perf->popDynamicInstructionInfo();
   return cost;
}

// Instruction

void Instruction::print() const
{
   ostringstream out;
   out << "Address(0x" << hex << m_address << dec << ") Size(" << m_size << ") : ";
   for (unsigned int i = 0; i < m_operands.size(); i++)
   {
      const Operand& o = m_operands[i];
      o.print(out);
   }
   LOG_PRINT("%s", out.str().c_str());
}

// Operand

void Operand::print(ostringstream& out) const
{
   // Type
   if (m_type == REG)
   {
      out << "REG-";
      // Value
      out << m_value << "-";
      // Direction
      if (m_direction == READ)
         out << "READ, ";
      else
         out << "WRITE, ";
   }
   else if (m_type == MEMORY)
   {
      out << "MEMORY-";
      // Direction
      if (m_direction == READ)
         out << "READ, ";
      else
         out << "WRITE, ";
   }
   else if (m_type == IMMEDIATE)
   {
      out << "IMMEDIATE-";
      // Value
      out << m_value << ", ";
   }
   else
   {
      LOG_PRINT_ERROR("Unrecognized Operand Type(%u)", m_type);
   }
}
