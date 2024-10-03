/*!
 * \file   mfront/src/CMaterialPropertyInterfaceBase.cxx
 * \brief
 * \author Thomas Helfer
 * \date   06 mai 2008
 * \copyright Copyright (C) 2006-2018 CEA/DEN, EDF R&D. All rights
 * reserved.
 * This project is publicly released under either the GNU GPL Licence with
 * linking exception or the CECILL-A licence. A copy of thoses licences are
 * delivered with the sources of TFEL. CEA or EDF may also distribute this
 * project under specific licensing conditions.
 */

#include <sstream>
#include <algorithm>
#include <stdexcept>
#include "TFEL/Raise.hxx"
#include "MFront/DSLUtilities.hxx"
#include "MFront/MFrontHeader.hxx"
#include "MFront/FileDescription.hxx"
#include "MFront/MaterialPropertyDescription.hxx"
#include "MFront/CMaterialPropertyInterfaceBase.hxx"

namespace mfront {

  static std::string transformHeaderName(const std::string& h) {
    auto replace = [](std::string& s, const std::string::value_type a,
                      const std::string::value_type b) {
      std::string::size_type p = 0;
      while ((p = s.find(a)) != std::string::npos) {
        s[p] = b;
      }
    };
    auto header = makeUpperCase(h);
    replace(header, '.', '_');
    replace(header, '-', '_');
    replace(header, '/', '_');
    return header;
  }  // end of transformHeaderName

  static std::string getOutputType(const MaterialPropertyDescription& mpd,
                                   const std::string floating_point_type,
                                   const bool use_qt) {
    if ((!use_qt) || (!useQuantities(mpd))) {
      return floating_point_type;
    }
    const auto& output_type = mpd.output.type;
    const auto scalar_types = getScalarTypeAliases();
    if (std::find(scalar_types.begin(), scalar_types.end(), output_type) ==
        scalar_types.end()) {
      tfel::raise(
          "CMaterialPropertyInterfaceBase::writeOutputFiles: "
          "unsupported output type");
    }
    return "typename tfel::config::ScalarTypes<" + floating_point_type +
           ", true>::" + output_type;
  }

  CMaterialPropertyInterfaceBase::CMaterialPropertyInterfaceBase() = default;

  std::vector<std::string>
  CMaterialPropertyInterfaceBase::getSupportedFloatingPointTypes() const {
    return {"double"};
  }  // end of getSupportedFloatingPointTypes

  bool
  CMaterialPropertyInterfaceBase::shallGenerateOverloadedFunctionForQuantities()
      const {
    return false;
  }  // end of shallGenerateOverloadedFunctionForQuantities

  void CMaterialPropertyInterfaceBase::writeHeaderPreprocessorDirectives(
      std::ostream& os, const MaterialPropertyDescription&) const {
    writeExportDirectives(os);
  }  // end of writeSrcPreprocessorDirectives

  void CMaterialPropertyInterfaceBase::writeSrcPreprocessorDirectives(
      std::ostream&, const MaterialPropertyDescription&) const {
  }  // end of writeSrcPreprocessorDirectives

  void CMaterialPropertyInterfaceBase::writeOutputFiles(
      const MaterialPropertyDescription& mpd, const FileDescription& fd) const {
    this->writeHeaderFile(mpd, fd);
    this->writeSrcFile(mpd, fd);
  }  // end of writeOutputFiles

  std::string CMaterialPropertyInterfaceBase::getCallingConvention() const {
    return "";
  }

  void CMaterialPropertyInterfaceBase::writeHeaderFile(
      const MaterialPropertyDescription& mpd, const FileDescription& fd) const {
    // the fortran interface does not need any header...
    const auto header = this->getHeaderFileName(mpd.material, mpd.className);
    if (header.empty()) {
      return;
    }
    const auto fn = "include/" + header + ".hxx";
    std::ofstream os(fn);
    tfel::raise_if(!os,
                   "CMaterialPropertyInterfaceBase::writeSrcFiles: "
                   "unable to open '" +
                       fn + "' for writing output file.");
    os.exceptions(std::ios::badbit | std::ios::failbit);
    const auto headerGard = transformHeaderName(header) + "_HH";
    os << "/*!\n"
       << "* \\file   " << fn << "\n"
       << "* \\brief  "
       << "this file declares the " << mpd.className << " MaterialLaw.\n"
       << "*         File generated by " << MFrontHeader::getVersionName()
       << " "
       << "version " << MFrontHeader::getVersionNumber() << "\n";
    if (!fd.authorName.empty()) {
      os << "* \\author " << fd.authorName << "\n";
    }
    if (!fd.date.empty()) {
      os << "* \\date   " << fd.date << "\n";
    }
    if (!fd.description.empty()) {
      os << fd.description << "\n";
    }
    os << " */\n\n"
       << "#ifndef " << headerGard << "\n"
       << "#define " << headerGard << "\n\n";

    if ((useQuantities(mpd)) &&
        (this->shallGenerateOverloadedFunctionForQuantities())) {
      os << "#include \"TFEL/Config/TFELTypes.hxx\"\n\n";
    }
    writeExportDirectives(os);
    this->writeHeaderPreprocessorDirectives(os, mpd);
    this->writeBeginHeaderNamespace(os);
    if (mpd.inputs.empty()) {
      const auto fptypes = this->getSupportedFloatingPointTypes();
      if (fptypes.size() == 1) {
        os << fptypes[0] << " " << this->getCallingConvention() << '\n'
           << this->getFunctionName(mpd) << "();\n\n";
      } else {
        const auto default_fp_type = [&fptypes]() -> std::string {
          if (std::find(fptypes.begin(), fptypes.end(), "double") !=
              fptypes.end()) {
            return "double";
          }
          if (std::find(fptypes.begin(), fptypes.end(), "long double") !=
              fptypes.end()) {
            return "long double";
          }
          return fptypes[0];
        }();
        if ((useQuantities(mpd)) &&
            (this->shallGenerateOverloadedFunctionForQuantities())) {
          os << "template <typename mfront_ValueType"  //
             << " = " << getOutputType(mpd, default_fp_type, true) << ">\n";
          os << "mfront_ValueType " << this->getCallingConvention() << '\n'
             << this->getFunctionName(mpd) << "();\n\n";
        } else {
          os << "template <typename mfront_ValueType"  //
             << " = " << default_fp_type << ">\n";
          os << "mfront_ValueType " << this->getCallingConvention() << '\n'
             << this->getFunctionName(mpd) << "();\n\n";
        }
        for (const auto& fptype : fptypes) {
          os << "template <>\n";
          os << fptype << " " << this->getCallingConvention() << '\n'
             << this->getFunctionName(mpd) << "<" << fptype << ">();\n\n";
          if ((useQuantities(mpd)) &&
              (this->shallGenerateOverloadedFunctionForQuantities())) {
            const auto output_type = getOutputType(mpd, fptype, true);
            os << "template <>\n";
            os << output_type << " " << this->getCallingConvention() << '\n'
               << this->getFunctionName(mpd) << "<" << output_type
               << ">();\n\n";
          }
        }
      }
      if (this->requiresCheckBoundsFunction()) {
        os << "MFRONT_SHAREDOBJ int " << this->getCallingConvention() << '\n'
           << this->getCheckBoundsFunctionName(mpd) << "();\n\n";
      }
    } else {
      for (const auto& floating_point_type :
           this->getSupportedFloatingPointTypes()) {
        os << "MFRONT_SHAREDOBJ " << floating_point_type << " "
           << this->getCallingConvention() << '\n'
           << this->getFunctionName(mpd);
        os << "(";
        this->writeArgumentsList(os, mpd, floating_point_type, false);
        os << ");\n\n";
        if ((useQuantities(mpd)) &&
            (this->shallGenerateOverloadedFunctionForQuantities())) {
          os << "MFRONT_SHAREDOBJ "
             << getOutputType(mpd, floating_point_type, true) << " "
             << this->getCallingConvention() << '\n'
             << this->getFunctionName(mpd);
          os << "(";
          this->writeArgumentsList(os, mpd, floating_point_type, true);
          os << ");\n\n";
        }
        if (((hasBounds(mpd.inputs)) || (hasPhysicalBounds(mpd.inputs))) ||
            (this->requiresCheckBoundsFunction())) {
          os << "MFRONT_SHAREDOBJ int " << this->getCallingConvention() << '\n'
             << this->getCheckBoundsFunctionName(mpd);
          os << "(";
          this->writeArgumentsList(os, mpd, floating_point_type, false);
          os << ");\n\n";
          if ((useQuantities(mpd)) &&
              (this->shallGenerateOverloadedFunctionForQuantities())) {
            os << "MFRONT_SHAREDOBJ int "  //
               << this->getCallingConvention() << '\n'
               << this->getCheckBoundsFunctionName(mpd);
            os << "(";
            this->writeArgumentsList(os, mpd, floating_point_type, true);
            os << ");\n\n";
          }
        }
      }
    }
    this->writeEndHeaderNamespace(os);
    os << "#endif /* " << headerGard << " */\n";
    os.close();
  }  // end of writeHeaderFile

  void CMaterialPropertyInterfaceBase::writeInterfaceSpecificVariables(
      std::ostream& os, const MaterialPropertyDescription& mpd) const {
    if ((!useQuantities(mpd)) && (!mpd.inputs.empty())) {
      return;
    }
    for (const auto& a : mpd.inputs) {
      os << "const auto " << a.name << " = "  //
         << a.type << "(mfront_" << a.name << ");\n";
    }
  }  // end of writeInterfaceSpecificVariables

  void CMaterialPropertyInterfaceBase::writeArgumentsList(
      std::ostream& os,
      const MaterialPropertyDescription& mpd,
      const std::string_view t,
      const bool b) const {
    const auto use_qt = useQuantities(mpd);
    if (!mpd.inputs.empty()) {
      for (auto p = mpd.inputs.begin(); p != mpd.inputs.end();) {
        const auto n = (use_qt ? "mfront_" : "") + p->name;
        const auto type = [&t, &p, b]() -> std::string {
          if (b) {
            const auto scalar_types = getScalarTypeAliases();
            if (std::find(scalar_types.begin(), scalar_types.end(), p->type) ==
                scalar_types.end()) {
              tfel::raise(
                  "CMaterialPropertyInterfaceBase::writeArgumentsList: "
                  "unsupported type for argument '" +
                  p->getExternalName() + "' (" + p->name + ")");
            }
            return "typename tfel::config::ScalarTypes<" + std::string{t} +
                   ", true>::" + p->type;
          }
          return std::string{t};
        }();
        os << "const " << type << " " << n;
        if ((++p) != mpd.inputs.end()) {
          os << ",";
        }
      }
    } else {
      os << "void";
    }
  }  // end of writeArgumentsList

  VariableDescriptionContainer::size_type
  CMaterialPropertyInterfaceBase::getVariableNumber(
      const MaterialPropertyDescription& mpd, const std::string& n) {
    VariableDescriptionContainer::size_type nbr = 1u;
    for (const auto& i : mpd.inputs) {
      if (i.name == n) {
        return nbr;
      }
      ++nbr;
    }
    tfel::raise(
        "CMaterialPropertyInterfaceBase::getVariableNumber: "
        "no inputs named '" +
        n + "'");
  }  // end of getVariableNumber

  static void writePhysicalBounds(std::ostream& out,
                                  const MaterialPropertyDescription& mpd) {
    for (const auto& i : mpd.inputs) {
      if (!i.hasPhysicalBounds()) {
        continue;
      }
      const auto& b = i.getPhysicalBounds();
      const auto nbr =
          CMaterialPropertyInterfaceBase::getVariableNumber(mpd, i.name);
      const auto cast_start = useQuantities(mpd) ? i.type + "(" : "";
      const auto cast_end = useQuantities(mpd) ? ")" : "";
      if (b.boundsType == VariableBoundsDescription::LOWER) {
        out << "if(" << i.name << " < " << cast_start << b.lowerBound
            << cast_end << "){\n"
            << "return -" << nbr << ";\n"
            << "}\n";
      } else if (b.boundsType == VariableBoundsDescription::UPPER) {
        out << "if(" << i.name << " > " << cast_start << b.upperBound
            << cast_end << "){\n"
            << "return -" << nbr << ";\n"
            << "}\n";
      } else {
        out << "if((" << i.name << " < " << cast_start << b.lowerBound
            << cast_end << ")||"
            << "(" << i.name << " > " << cast_start << b.upperBound << cast_end
            << ")){\n"
            << "return -" << nbr << ";\n"
            << "}\n";
      }
    }
  }

  static void writeBounds(std::ostream& out,
                          const MaterialPropertyDescription& mpd) {
    for (const auto& i : mpd.inputs) {
      if (!i.hasBounds()) {
        continue;
      }
      const auto& b = i.getBounds();
      const auto nbr =
          CMaterialPropertyInterfaceBase::getVariableNumber(mpd, i.name);
      const auto cast_start = useQuantities(mpd) ? i.type + "(" : "";
      const auto cast_end = useQuantities(mpd) ? ")" : "";
      if (b.boundsType == VariableBoundsDescription::LOWER) {
        out << "if(" << i.name << " < " << cast_start << b.lowerBound
            << cast_end << "){\n"
            << "return " << nbr << ";\n"
            << "}\n";
      } else if (b.boundsType == VariableBoundsDescription::UPPER) {
        out << "if(" << i.name << " > " << cast_start << b.upperBound
            << cast_end << "){\n"
            << "return " << nbr << ";\n"
            << "}\n";
      } else {
        out << "if((" << i.name << " < " << cast_start << b.lowerBound
            << cast_end << ")||"
            << "(" << i.name << " > " << cast_start << b.upperBound << cast_end
            << ")){\n"
            << "return " << nbr << ";\n"
            << "}\n";
      }
    }
  }

  std::string CMaterialPropertyInterfaceBase::getSymbolName(
      const MaterialPropertyDescription& mpd) const {
    const auto material = mpd.material;
    const auto className = mpd.className;
    return material.empty() ? className : material + "_" + className;
  }  // end of getSymbolName

  void CMaterialPropertyInterfaceBase::writeFileDescriptionSymbols(
      std::ostream& os,
      const MaterialPropertyDescription& mpd,
      const FileDescription& fd) const {
    mfront::writeFileDescriptionSymbols(os, this->getSymbolName(mpd), fd);
  }  // end of writeFileDescriptionSymbols

  void CMaterialPropertyInterfaceBase::writeValidatorSymbol(
      std::ostream& os, const MaterialPropertyDescription& mpd) const {
    mfront::writeValidatorSymbol(os, this->getSymbolName(mpd), mpd);
  }  // end of writeValidatorSymbol

  void CMaterialPropertyInterfaceBase::writeBuildIdentifierSymbol(
      std::ostream& os, const MaterialPropertyDescription& mpd) const {
    mfront::writeBuildIdentifierSymbol(os, this->getSymbolName(mpd), mpd);
  }  // end of writeBuildIdentifierSymbol

  void CMaterialPropertyInterfaceBase::writeEntryPointSymbol(
      std::ostream& os, const MaterialPropertyDescription& mpd) const {
    mfront::writeEntryPointSymbol(os, this->getSymbolName(mpd));
  }  // end of writeEntryPointSymbol

  void CMaterialPropertyInterfaceBase::writeTFELVersionSymbol(
      std::ostream& os, const MaterialPropertyDescription& mpd) const {
    mfront::writeTFELVersionSymbol(os, this->getSymbolName(mpd));
  }  // end of writeTFELVersionSymbol

  void CMaterialPropertyInterfaceBase::writeUnitSystemSymbol(
      std::ostream& os, const MaterialPropertyDescription& mpd) const {
    mfront::writeUnitSystemSymbol(os, this->getSymbolName(mpd), mpd);
  }  // end of writeUnitSystemSymbol

  void CMaterialPropertyInterfaceBase::writeLawSymbol(
      std::ostream& os, const MaterialPropertyDescription& mpd) const {
    mfront::writeLawSymbol(os, this->getSymbolName(mpd), mpd.law);
  }  // end of writeLawSymbol

  void CMaterialPropertyInterfaceBase::writeMaterialSymbol(
      std::ostream& os, const MaterialPropertyDescription& mpd) const {
    mfront::writeMaterialSymbol(os, this->getSymbolName(mpd), mpd.material);
  }  // end of writeMaterialSymbol

  void CMaterialPropertyInterfaceBase::writeMaterialKnowledgeTypeSymbol(
      std::ostream& os, const MaterialPropertyDescription& mpd) const {
    mfront::writeMaterialKnowledgeTypeSymbol(
        os, this->getSymbolName(mpd), MaterialKnowledgeType::MATERIALPROPERTY);
  }  // end of writeMaterialKnowledgeTypeSymbol

  void CMaterialPropertyInterfaceBase::writeVariablesNamesSymbol(
      std::ostream& os, const MaterialPropertyDescription& mpd) const {
    mfront::writeVariablesNamesSymbol(os, this->getSymbolName(mpd), mpd);
  }  // end of writeVariablesNamesSymbol

  void CMaterialPropertyInterfaceBase::writeVariablesBoundsSymbols(
      std::ostream& os, const MaterialPropertyDescription& mpd) const {
    mfront::writeVariablesBoundsSymbols(os, this->getSymbolName(mpd), mpd);
  }  // end of writeVariablesBoundsSymbols

  void CMaterialPropertyInterfaceBase::writeSrcFile(
      const MaterialPropertyDescription& mpd, const FileDescription& fd) const {
    // opening the source file
    const auto src = this->getSrcFileName(mpd.material, mpd.className);
    const auto fn = "src/" + src + ".cxx";
    std::ofstream os(fn);
    tfel::raise_if(!os,
                   "CMaterialPropertyInterfaceBase::writeSrcFiles: "
                   "unable to open file '" +
                       fn + "' for writing output file.");
    os.exceptions(std::ios::badbit | std::ios::failbit);
    os << "/*!\n"
       << " * \\file   " << fn << '\n'
       << " * \\brief  "
       << "this file implements the " << mpd.className << " MaterialLaw.\n"
       << " *         File generated by " << MFrontHeader::getVersionName()
       << " "
       << "version " << MFrontHeader::getVersionNumber() << '\n';
    if (!fd.authorName.empty()) {
      os << "* \\fd.author " << fd.authorName << '\n';
    }
    if (!fd.date.empty()) {
      os << "* \\date   " << fd.date << '\n';
    }
    os << " */\n\n"
       << "#include<cmath>\n"
       << "#include<cerrno>\n"
       << "#include<algorithm>\n"
       << "#include\"TFEL/Config/TFELTypes.hxx\"\n"
       << "#include\"TFEL/PhysicalConstants.hxx\"\n"
       << "#include\"TFEL/Math/General/IEEE754.hxx\"\n\n";
    if (useQuantities(mpd)) {
      os << "#include\"TFEL/Math/qt.hxx\"\n\n"
         << "#include\"TFEL/Math/Quantity/qtIO.hxx\"\n\n";
    }
    if (!mpd.includes.empty()) {
      os << mpd.includes << "\n\n";
    }
    const auto header = this->getHeaderFileName(mpd.material, mpd.className);
    if (!header.empty()) {
      os << "#include\"" + header + ".hxx\"\n\n";
    }
    this->writeSrcPreprocessorDirectives(os, mpd);
    //
    this->writeFileDescriptionSymbols(os, mpd, fd);
    this->writeVariablesNamesSymbol(os, mpd);
    this->writeVariablesBoundsSymbols(os, mpd);
    this->writeBuildIdentifierSymbol(os, mpd);
    this->writeEntryPointSymbol(os, mpd);
    this->writeTFELVersionSymbol(os, mpd);
    this->writeUnitSystemSymbol(os, mpd);
    this->writeInterfaceSymbol(os, mpd);
    this->writeLawSymbol(os, mpd);
    this->writeMaterialSymbol(os, mpd);
    this->writeMaterialKnowledgeTypeSymbol(os, mpd);
    //
    this->writeBeginSrcNamespace(os);
    if (mpd.inputs.empty()) {
      const auto fptypes = this->getSupportedFloatingPointTypes();
      if (fptypes.size() == 1) {
        os << fptypes[0] << " " << this->getCallingConvention() << '\n'
           << this->getFunctionName(mpd) << "(){\n";
        this->writeMaterialPropertyBody(os, mpd, fd, fptypes[0], false);
        os << "} /* end of " << mpd.className << " */\n\n";
      } else {
        for (const auto& fptype : this->getSupportedFloatingPointTypes()) {
          os << "template<>\n"
             << fptype << " " << this->getCallingConvention() << '\n'
             << this->getFunctionName(mpd) << "<" << fptype << ">(){\n";
          this->writeMaterialPropertyBody(os, mpd, fd, fptype, false);
          os << "} /* end of " << mpd.className << " */\n\n";
          if ((useQuantities(mpd)) &&
              (this->shallGenerateOverloadedFunctionForQuantities())) {
            const auto output_type = getOutputType(mpd, fptype, true);
            os << "template <>\n";
            os << output_type << " " << this->getCallingConvention() << '\n'
               << this->getFunctionName(mpd) << "<" << output_type << ">(){\n";
            this->writeMaterialPropertyBody(os, mpd, fd, fptype, true);
            os << "} /* end of " << mpd.className << " */\n\n";
          }
        }
      }
    } else {
      for (const auto& floating_point_type :
           this->getSupportedFloatingPointTypes()) {
        os << floating_point_type << " " << this->getFunctionName(mpd) << "(";
        this->writeArgumentsList(os, mpd, floating_point_type, false);
        os << ")\n{\n";
        this->writeMaterialPropertyBody(os, mpd, fd, floating_point_type,
                                        false);
        os << "} /* end of " << mpd.className << " */\n\n";
        if ((useQuantities(mpd)) &&
            (this->shallGenerateOverloadedFunctionForQuantities())) {
          os << getOutputType(mpd, floating_point_type, true) << " "
             << this->getFunctionName(mpd) << "(";
          this->writeArgumentsList(os, mpd, floating_point_type, true);
          os << ")\n{\n";
          this->writeMaterialPropertyBody(os, mpd, fd, floating_point_type,
                                          true);
          os << "} /* end of " << mpd.className << " */\n\n";
        }
      }
    }
    if (mpd.inputs.empty()) {
      if (this->requiresCheckBoundsFunction()) {
        os << "int " << this->getCheckBoundsFunctionName(mpd) << "(){";
        this->writeMaterialPropertyCheckBoundsBody(os, mpd, "double");
        os << "} /* end of " << mpd.className << "_checkBounds */\n\n";
      }
    } else {
      for (const auto& floating_point_type :
           this->getSupportedFloatingPointTypes()) {
        if (((hasBounds(mpd.inputs)) || (hasPhysicalBounds(mpd.inputs))) ||
            (this->requiresCheckBoundsFunction())) {
          os << "int " << this->getCheckBoundsFunctionName(mpd);
          os << "(";
          this->writeArgumentsList(os, mpd, floating_point_type, false);
          os << ")\n{\n";
          this->writeMaterialPropertyCheckBoundsBody(os, mpd,
                                                     floating_point_type);
          os << "} /* end of " << mpd.className << "_checkBounds */\n\n";
        }
      }
    }
    this->writeEndSrcNamespace(os);
    os.close();
  }  // end of writeSrcFile

  void CMaterialPropertyInterfaceBase::writeMaterialPropertyBody(
      std::ostream& os,
      const MaterialPropertyDescription& mpd,
      const FileDescription& fd,
      const std::string_view floating_point_type,
      const bool use_qt) const {
    writeBeginningOfMaterialPropertyBody(os, mpd, fd, floating_point_type,
                                         true);
    // parameters
    if (!mpd.parameters.empty()) {
      for (const auto& p : mpd.parameters) {
        tfel::raise_if(!p.hasAttribute(VariableDescription::defaultValue),
                       "CMaterialPropertyInterfaceBase::writeSrcFile : "
                       "internal error (can't find value of "
                       "parameter '" +
                           p.name + "')");
        os << "static constexpr auto " << p.name << " = " << p.type << "("
           << p.getAttribute<double>(VariableDescription::defaultValue)
           << ");\n";
      }
    }
    this->writeInterfaceSpecificVariables(os, mpd);
    if ((!areRuntimeChecksDisabled(mpd)) && (!mpd.inputs.empty())) {
      os << "#ifndef MFRONT_NOERRNO_HANDLING\n"
         << "const auto mfront_errno_old = errno;\n"
         << "errno=0;\n"
         << "#endif /* MFRONT_NOERRNO_HANDLING */\n";
    }
    os << "auto " << mpd.output.name << " = " << mpd.output.type << "{};\n";
    this->writeCxxTryBlock(os);
    os << mpd.f.body << "\n";
    if (!areRuntimeChecksDisabled(mpd)) {
      // checking the bounds and physical bounds of the output
      if ((mpd.output.hasBounds()) || (mpd.output.hasPhysicalBounds())) {
        const auto cast_start = useQuantities(mpd) ? mpd.output.type + "(" : "";
        const auto cast_end = useQuantities(mpd) ? ")" : "";
        if (mpd.output.hasPhysicalBounds()) {
          const auto b = mpd.output.getPhysicalBounds();
          if (b.boundsType == VariableBoundsDescription::LOWER) {
            os << "if(" << mpd.output.name << " < " << cast_start
               << b.lowerBound << cast_end << "){\n"
               << "return std::nan(\"\");\n"
               << "}\n";
          } else if (b.boundsType == VariableBoundsDescription::UPPER) {
            os << "if(" << mpd.output.name << " > " << cast_start
               << b.upperBound << cast_end << "){\n"
               << "return std::nan(\"\");\n"
               << "}\n";
          } else {
            os << "if((" << mpd.output.name << " < " << cast_start
               << b.lowerBound << cast_end << ")||"
               << "(" << mpd.output.name << " > " << cast_start << b.upperBound
               << cast_end << ")){\n"
               << "return std::nan(\"\");\n"
               << "}\n";
          }
        }
      }
    }
    this->writeCxxCatchBlock(os, mpd, floating_point_type, use_qt);
    if ((!areRuntimeChecksDisabled(mpd)) && (!mpd.inputs.empty())) {
      os << "#ifndef MFRONT_NOERRNO_HANDLING\n";
      // can't use std::swap here as errno might be a macro
      os << "const auto mfront_errno = errno;\n"
         << "errno = mfront_errno_old;\n";
      if (use_qt) {
        os << "if((mfront_errno != 0)||"
           << "(!tfel::math::ieee754::isfinite(" << mpd.output.name
           << ".getValue()))){\n";
      } else {
        os << "if((mfront_errno != 0)||"
           << "(!tfel::math::ieee754::isfinite(" << mpd.output.name << "))){\n";
      }
      this->writeCErrorTreatment(os, mpd, floating_point_type, use_qt);
      os << "}\n"
         << "#endif /* MFRONT_NOERRNO_HANDLING */\n";
    }
    if ((useQuantities(mpd)) && (!use_qt)) {
      os << "return " << mpd.output.name << ".getValue();\n";
    } else {
      os << "return " << mpd.output.name << ";\n";
    }
  }  // end of writeMaterialPropertyBody

  void CMaterialPropertyInterfaceBase::writeMaterialPropertyCheckBoundsBody(
      std::ostream& os,
      const MaterialPropertyDescription& mpd,
      const std::string_view floating_point_type) const {
    writeScalarStandardTypedefs(os, mpd, floating_point_type, true);
    this->writeInterfaceSpecificVariables(os, mpd);
    for (const auto& i : mpd.inputs) {
      os << "static_cast<void>(" << i.name << ");\n";
    }
    if (!areRuntimeChecksDisabled(mpd)) {
      if (hasPhysicalBounds(mpd.inputs)) {
        os << "/* treating mpd.physical bounds */\n";
        writePhysicalBounds(os, mpd);
      }
      if (hasBounds(mpd.inputs)) {
        os << "/* treating standard bounds */\n";
        writeBounds(os, mpd);
      }
    }
    os << "return 0;\n";
  }  // end of writeMaterialPropertyCheckBoundsBody

  void CMaterialPropertyInterfaceBase::writeCxxTryBlock(
      std::ostream& os) const {
    os << "try{\n";
  }  // end of writeCxxCatchBlock

  void CMaterialPropertyInterfaceBase::writeCxxCatchBlock(
      std::ostream& os,
      const MaterialPropertyDescription& mpd,
      const std::string_view floating_point_type,
      const bool use_qt) const {
    os << "} catch(std::exception&){\n";
    this->returnInvalidValue(os, "e.what()", mpd, floating_point_type, use_qt);
    os << "} catch(...){\n";
    this->returnInvalidValue(os, "\"unsupported C++ exception\"", mpd,
                             floating_point_type, use_qt);
    os << "}\n";
  }  // end of writeCxxCatchBlock

  void CMaterialPropertyInterfaceBase::writeCErrorTreatment(
      std::ostream& os,
      const MaterialPropertyDescription& mpd,
      const std::string_view floating_point_type,
      const bool use_qt) const {
    const auto* msg = "\"invalid call to a C function (errno is not null)\"";
    this->returnInvalidValue(os, msg, mpd, floating_point_type, use_qt);
  }  // end of writeCErrorTreatment

  void CMaterialPropertyInterfaceBase::returnInvalidValue(
      std::ostream& os,
      std::string_view,
      const MaterialPropertyDescription& mpd,
      const std::string_view floating_point_type,
      const bool use_qt) const {
    os << "return ";
    if (use_qt) {
      os << getOutputType(mpd, std::string{floating_point_type}, true) << "{";
    }
    os << "std::nan(\"\")";
    if (use_qt) {
      os << "}";
    }
    os << ";\n";
  }  // end of returnInvalidValue

  CMaterialPropertyInterfaceBase::~CMaterialPropertyInterfaceBase() = default;

}  // end of namespace mfront
